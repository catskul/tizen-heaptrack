/*
 * Copyright 2014-2017 Milian Wolff <mail@milianw.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file heaptrack_interpret.cpp
 *
 * @brief Interpret raw heaptrack data and add Dwarf based debug information.
 */

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <sstream>
#include <stdio_ext.h>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <cxxabi.h>

#include <boost/algorithm/string/predicate.hpp>

#include "libbacktrace/backtrace.h"
#include "libbacktrace/internal.h"
#include "util/linereader.h"
#include "util/pointermap.h"

#include <unistd.h>

using namespace std;

namespace {

string demangle(const char* function)
{
    if (!function) {
        return {};
    } else if (function[0] != '_' || function[1] != 'Z') {
        return {function};
    }

    string ret;
    int status = 0;
    char* demangled = abi::__cxa_demangle(function, 0, 0, &status);
    if (demangled) {
        ret = demangled;
        free(demangled);
    }
    return ret;
}

struct Frame
{
    Frame(string function = {}, string file = {}, int line = 0, size_t moduleOffset = 0)
        : function(function)
        , file(file)
        , line(line)
    {}

    bool isValid() const
    {
        return !function.empty();
    }

    string function;
    string file;
    int line;
    size_t moduleOffset;
};

struct AddressInformation
{
    Frame frame;
    vector<Frame> inlined;
};

struct ResolvedFrame
{
    ResolvedFrame(size_t functionIndex = 0, size_t fileIndex = 0, int line = 0, size_t moduleOffset = 0)
        : functionIndex(functionIndex)
        , fileIndex(fileIndex)
        , line(line)
        , moduleOffset(moduleOffset)
    {}
    size_t functionIndex;
    size_t fileIndex;
    int line;
    size_t moduleOffset;
};

struct ResolvedIP
{
    size_t moduleIndex = 0;
    ResolvedFrame frame;
    vector<ResolvedFrame> inlined;
};

struct Module
{
    Module(uintptr_t addressStart, uintptr_t addressEnd, backtrace_state* backtraceState, size_t moduleIndex)
        : addressStart(addressStart)
        , addressEnd(addressEnd)
        , moduleIndex(moduleIndex)
        , backtraceState(backtraceState)
    {
    }

    AddressInformation resolveAddress(uintptr_t address) const
    {
        AddressInformation info;
        if (!backtraceState) {
            return info;
        }

        // try to find frame information from debug information
        backtrace_pcinfo(backtraceState, address,
                         [](void* data, uintptr_t /*addr*/, const char* file, int line, const char* function) -> int {
                             Frame frame(demangle(function), file ? file : "", line);
                             auto info = reinterpret_cast<AddressInformation*>(data);
                             if (!info->frame.isValid()) {
                                info->frame = frame;
                             } else {
                                info->inlined.push_back(frame);
                             }
                             return 0;
                         },
                         [](void* /*data*/, const char* /*msg*/, int /*errnum*/) {}, &info);

        // no debug information available? try to fallback on the symbol table information
        if (!info.frame.isValid()) {
            backtrace_syminfo(
                backtraceState, address,
                [](void* data, uintptr_t /*pc*/, const char* symname, uintptr_t /*symval*/, uintptr_t /*symsize*/) {
                    if (symname) {
                        reinterpret_cast<AddressInformation*>(data)->frame.function = demangle(symname);
                    }
                },
                [](void* /*data*/, const char* msg, int errnum) {
                    cerr << "Module backtrace error (code " << errnum << "): " << msg << endl;
                },
                &info);
        }

        info.frame.moduleOffset = (address - addressStart);

        return info;
    }

    bool operator<(const Module& module) const
    {
        return tie(addressStart, addressEnd, moduleIndex)
            < tie(module.addressStart, module.addressEnd, module.moduleIndex);
    }

    bool operator!=(const Module& module) const
    {
        return tie(addressStart, addressEnd, moduleIndex)
            != tie(module.addressStart, module.addressEnd, module.moduleIndex);
    }

    uintptr_t addressStart;
    uintptr_t addressEnd;
    size_t moduleIndex;
    backtrace_state* backtraceState;
};

struct AccumulatedTraceData
{
    AccumulatedTraceData()
    {
        m_modules.reserve(256);
        m_backtraceStates.reserve(64);
        m_internedData.reserve(4096);
        m_encounteredIps.reserve(32768);
        m_encounteredClasses.reserve(4096);
    }

    ~AccumulatedTraceData()
    {
        fprintf(stdout, "# strings: %zu\n# ips: %zu\n", m_internedData.size(), m_encounteredIps.size());
    }

    ResolvedIP resolve(const uintptr_t ip)
    {
        if (m_modulesDirty) {
            // sort by addresses, required for binary search below
            sort(m_modules.begin(), m_modules.end());

#ifndef NDEBUG
            for (size_t i = 0; i < m_modules.size(); ++i) {
                const auto& m1 = m_modules[i];
                for (size_t j = i + 1; j < m_modules.size(); ++j) {
                    if (i == j) {
                        continue;
                    }
                    const auto& m2 = m_modules[j];
                    if ((m1.addressStart <= m2.addressStart && m1.addressEnd > m2.addressStart)
                        || (m1.addressStart < m2.addressEnd && m1.addressEnd >= m2.addressEnd)) {
                        cerr << "OVERLAPPING MODULES: " << hex << m1.moduleIndex << " (" << m1.addressStart << " to "
                             << m1.addressEnd << ") and " << m1.moduleIndex << " (" << m2.addressStart << " to "
                             << m2.addressEnd << ")\n"
                             << dec;
                    } else if (m2.addressStart >= m1.addressEnd) {
                        break;
                    }
                }
            }
#endif

            m_modulesDirty = false;
        }

        auto resolveFrame = [this](const Frame& frame)
        {
            return ResolvedFrame{intern(frame.function), intern(frame.file), frame.line, frame.moduleOffset};
        };

        ResolvedIP data;
        // find module for this instruction pointer
        auto module =
            lower_bound(m_modules.begin(), m_modules.end(), ip,
                        [](const Module& module, const uintptr_t ip) -> bool { return module.addressEnd < ip; });
        if (module != m_modules.end() && module->addressStart <= ip && module->addressEnd >= ip) {
            data.moduleIndex = module->moduleIndex;
            const auto info = module->resolveAddress(ip);
            data.frame = resolveFrame(info.frame);
            std::transform(info.inlined.begin(), info.inlined.end(), std::back_inserter(data.inlined),
                           resolveFrame);
        }
        return data;
    }

    size_t intern(const string& str, std::string* internedString = nullptr)
    {
        if (str.empty()) {
            return 0;
        }

        auto it = m_internedData.find(str);
        if (it != m_internedData.end()) {
            if (internedString) {
                *internedString = it->first;
            }
            return it->second;
        }
        const size_t id = m_internedData.size() + 1;
        it = m_internedData.insert(it, make_pair(str, id));
        if (internedString) {
            *internedString = it->first;
        }
        fprintf(stdout, "s %s\n", str.c_str());
        return id;
    }

    void addModule(backtrace_state* backtraceState, const size_t moduleIndex, const uintptr_t addressStart,
                   const uintptr_t addressEnd)
    {
        m_modules.emplace_back(addressStart, addressEnd, backtraceState, moduleIndex);
        m_modulesDirty = true;
    }

    void clearModules()
    {
        // TODO: optimize this, reuse modules that are still valid
        m_modules.clear();
        m_modulesDirty = true;
    }

    void addManagedNameForIP(uintptr_t ip, string managedName)
    {
        m_managedNames.insert(std::make_pair(ip, managedName));
    }

    size_t addIp(const uintptr_t instructionPointer, bool isManaged)
    {
        if (!instructionPointer) {
            return 0;
        }

        auto it = m_encounteredIps.find(instructionPointer);
        if (it != m_encounteredIps.end()) {
            return it->second;
        }

        const size_t ipId = m_encounteredIps.size() + 1;
        m_encounteredIps.insert(it, make_pair(instructionPointer, ipId));

        if (isManaged) {
            size_t functionIndex = intern(m_managedNames[instructionPointer]);

            fprintf(stdout, "i %llx 1 0 0 %zx\n", (1ull << 63) | instructionPointer, functionIndex);
        } else {
            const auto ip = resolve(instructionPointer);
            fprintf(stdout, "i %zx 0 %zx %zx", instructionPointer, ip.moduleIndex, ip.frame.moduleOffset);
            if (ip.frame.functionIndex || ip.frame.fileIndex) {
                fprintf(stdout, " %zx", ip.frame.functionIndex);
                if (ip.frame.fileIndex) {
                    fprintf(stdout, " %zx %x", ip.frame.fileIndex, ip.frame.line);
                    for (const auto& inlined : ip.inlined) {
                        fprintf(stdout, " %zx %zx %x", inlined.functionIndex, inlined.fileIndex, inlined.line);
                    }
                }
            }
            fputc('\n', stdout);
        }

        return ipId;
    }

    size_t addClass(const uintptr_t classPointer) {
        if (!classPointer) {
            return 0;
        }

        auto it = m_encounteredClasses.find(classPointer);
        if (it != m_encounteredClasses.end()) {
            return it->second;
        }

        size_t classIndex = intern(m_managedNames[classPointer]);
        m_encounteredClasses.insert(it, make_pair(classPointer, classIndex));

        fprintf(stdout, "C %zx\n", classIndex);
        return classIndex;
    }

    bool fileIsReadable(const string& path) const
    {
        return access(path.c_str(), R_OK) == 0;
    }

    std::string findDebugFile(const std::string& input) const
    {
        // TODO: also try to find a debug file by build-id
        // TODO: also lookup in (user-configurable) debug path
        std::string file = input + ".debug";
        return fileIsReadable(file) ? file : input;
    }

    std::string findBuildIdFile(const string& buildId) const
    {
        if (buildId.empty()) {
            return {};
        }
        // TODO: also lookup in (user-configurable) debug path
        const auto path = "/usr/lib/debug/.build-id/" + buildId.substr(0, 2) + '/' + buildId.substr(2) + ".debug";
        cerr << path << endl;
        return fileIsReadable(path) ? path : string();
    }

    /**
     * Prevent the same file from being initialized multiple times.
     * This drastically cuts the memory consumption down
     */
    backtrace_state* findBacktraceState(const std::string& originalFileName, const string& buildId, uintptr_t addressStart)
    {
        if (boost::algorithm::starts_with(originalFileName, "linux-vdso.so")) {
            // prevent warning, since this will always fail
            return nullptr;
        }

        auto it = m_backtraceStates.find(originalFileName);
        if (it != m_backtraceStates.end()) {
            return it->second;
        }

        // TODO: also lookup in (user-configurable) sysroot path
        const auto buildIdFile = findBuildIdFile(buildId);
        const auto fileName = buildIdFile.empty() ? findDebugFile(originalFileName) : buildIdFile;

        struct CallbackData
        {
            const char* fileName;
        };
        CallbackData data = {fileName.c_str()};

        auto errorHandler = [](void* rawData, const char* msg, int errnum) {
            auto data = reinterpret_cast<const CallbackData*>(rawData);
            cerr << "Failed to create backtrace state for module " << data->fileName << ": " << msg << " / "
                 << strerror(errnum) << " (error code " << errnum << ")" << endl;
        };

        auto state = backtrace_create_state(data.fileName, /* we are single threaded, so: not thread safe */ false,
                                            errorHandler, &data);

        if (state) {
            const int descriptor = backtrace_open(data.fileName, errorHandler, &data, nullptr);
            if (descriptor >= 1) {
                int foundSym = 0;
                int foundDwarf = 0;
                auto ret = elf_add(state, descriptor, addressStart, errorHandler, &data, &state->fileline_fn, &foundSym,
                                   &foundDwarf, false);
                if (ret && foundSym) {
                    state->syminfo_fn = &elf_syminfo;
                }
            }
        }

        m_backtraceStates.insert(it, make_pair(originalFileName, state));

        return state;
    }

private:
    vector<Module> m_modules;
    unordered_map<std::string, backtrace_state*> m_backtraceStates;
    bool m_modulesDirty = false;

    unordered_map<uintptr_t, string> m_managedNames;
    unordered_map<string, size_t> m_internedData;
    unordered_map<uintptr_t, size_t> m_encounteredIps;
    unordered_map<uintptr_t, size_t> m_encounteredClasses;
};
}

int main(int /*argc*/, char** /*argv*/)
{
    // optimize: we only have a single thread
    ios_base::sync_with_stdio(false);
    __fsetlocking(stdout, FSETLOCKING_BYCALLER);
    __fsetlocking(stdin, FSETLOCKING_BYCALLER);

    AccumulatedTraceData data;

    unordered_map<string, int> managed_name_ids;

    LineReader reader;

    string exe;

    PointerMap ptrToIndex;
    uint64_t lastPtr = 0;
    AllocationInfoSet allocationInfos;
    std::set<uint64_t> managedPointersSet;
    std::set<uint64_t> gcManagedPointersSet;

    bool isGCInProcess = false;

    uint64_t allocations = 0;
    uint64_t leakedAllocations = 0;
    uint64_t managedAllocations = 0;
    uint64_t leakedManagedAllocations = 0;
    uint64_t temporaryAllocations = 0;

    while (reader.getLine(cin)) {
        if (reader.mode() == 'x') {
            reader >> exe;
        } else if (reader.mode() == 'm') {
            string fileName;
            reader >> fileName;
            if (fileName == "-") {
                data.clearModules();
            } else {
                string buildId;
                reader >> buildId;

                uintptr_t addressStart = 0;
                if (!(reader >> addressStart)) {
                    cerr << "[C] failed to parse line: " << reader.line() << endl;
                    return 1;
                }

                if (fileName == "x") {
                    fileName = exe;
                }
                std::string internedString;
                const auto moduleIndex = data.intern(fileName, &internedString);

                auto state = data.findBacktraceState(internedString, buildId, addressStart);
                uintptr_t vAddr = 0;
                uintptr_t memSize = 0;
                while ((reader >> vAddr) && (reader >> memSize)) {
                    data.addModule(state, moduleIndex, addressStart + vAddr, addressStart + vAddr + memSize);
                }
            }
        } else if (reader.mode() == 't') {
            uintptr_t instructionPointer = 0;
            size_t parentIndex = 0;
            int is_managed;
            if (!(reader >> instructionPointer) || !(reader >> parentIndex) || !(reader >> is_managed)) {
                cerr << "[C] failed to parse line: " << reader.line() << endl;
                return 1;
            }
            // ensure ip is encountered
            const auto ipId = data.addIp(instructionPointer, is_managed);
            // trace point, map current output index to parent index
            fprintf(stdout, "t %zx %zx\n", ipId, parentIndex);
        } else if (reader.mode() == '^') {
            if (isGCInProcess) {
                cerr << "[W] wrong trace format (allocation during GC - according to Book of the Runtime, concurrent GC is turned off in case profiling is enabled)" << endl;
                continue;
            }

            ++managedAllocations;
            ++leakedManagedAllocations;
            uint64_t size = 0;
            TraceIndex traceId;
            uint64_t ptr = 0;
            if (!(reader >> traceId.index) || !(reader >> size) || !(reader >> ptr)) {
                cerr << "[W] failed to parse line: " << reader.line() << endl;
                continue;
            }

            AllocationIndex index;
            if (allocationInfos.add(size, traceId, &index, 1)) {
                fprintf(stdout, "a %" PRIx64 " %x 1\n", size, traceId.index);
            }
            ptrToIndex.addPointer(ptr, index);
            managedPointersSet.insert(ptr);
            lastPtr = ptr;
            fprintf(stdout, "^ %x\n", index.index);
        } else if (reader.mode() == 'G') {
            int isStart;

            if (!(reader >> isStart) || !(isStart == 1 || isStart == 0)) {
                cerr << "[W] failed to parse line: " << reader.line() << endl;
                continue;
            }

            if (isStart)
            {
                // GC chunk start
                if (isGCInProcess) {
                    cerr << "[W] wrong trace format (nested GC chunks)" << endl;
                    continue;
                }

                isGCInProcess = true;

                assert (gcManagedPointersSet.size() == 0);
            }
            else
            {
                assert (!isStart);

                // GC chunk end
                if (!isGCInProcess) {
                    cerr << "[W] wrong trace format (nested GC chunks?)" << endl;
                    continue;
                }

                isGCInProcess = false;

                for (auto managedPtr : managedPointersSet) {
                    auto allocation = ptrToIndex.takePointer(managedPtr);

                    if (!allocation.second) {
                        cerr << "[W] wrong trace format (unknown managed pointer) 0x" << std::hex << managedPtr << std::dec << endl;
                        continue;
                    }

                    fprintf(stdout, "~ %x\n", allocation.first.index);

                    --leakedManagedAllocations;
                }

                managedPointersSet = std::move(gcManagedPointersSet);

                assert (gcManagedPointersSet.size() == 0);
            }
        } else if (reader.mode() == 'L') {
            if (!isGCInProcess) {
                cerr << "[W] wrong trace format (range survival event when no GC is running)" << endl;
                continue;
            }

            uint64_t rangeLength, rangeStart, rangeMovedTo;

            if (!(reader >> rangeLength) || !(reader >> rangeStart) || !(reader >> rangeMovedTo)) {
                cerr << "[W] failed to parse line: " << reader.line() << endl;
                continue;
            }

            uint64_t targetRangeStart = (rangeMovedTo != 0 ? rangeMovedTo : rangeStart);
            uint64_t targetRangeEnd   = targetRangeStart + rangeLength;

            {
                auto it = gcManagedPointersSet.lower_bound(targetRangeStart);

                if (it != gcManagedPointersSet.end() && *it < targetRangeEnd) {
                    cerr << "[W] wrong trace format (survival ranges are intersecting during a GC session)" << endl;
                    continue;
                }
            }

            auto itFromBegin = managedPointersSet.lower_bound(rangeStart);
            auto itFromEnd   = managedPointersSet.lower_bound(rangeStart + rangeLength);

            if (targetRangeStart == rangeStart) {
                gcManagedPointersSet.insert(itFromBegin, itFromEnd);
            } else {
                for (auto itFrom = itFromBegin; itFrom != itFromEnd; ++itFrom) {
                    uint64_t source = *itFrom;
                    uint64_t target = targetRangeStart + (source - rangeStart);

                    assert(gcManagedPointersSet.find(target) == gcManagedPointersSet.end());

                    auto allocationAtTarget = ptrToIndex.takePointer(target);

                    if (allocationAtTarget.second) {
                        assert(managedPointersSet.find(target) != managedPointersSet.end());
                        assert(target < rangeStart || target >= rangeStart + rangeLength);

                        managedPointersSet.erase(target);

                        fprintf(stdout, "~ %x\n", allocationAtTarget.first.index);

                        --leakedManagedAllocations;
                    }

                    gcManagedPointersSet.insert(target);

                    auto allocation = ptrToIndex.takePointer(source);

                    assert(allocation.second);

                    ptrToIndex.addPointer(target, allocation.first);
                }
            }

            managedPointersSet.erase(itFromBegin, itFromEnd);

            for (auto ptr : managedPointersSet) {
                if(gcManagedPointersSet.find(ptr) != gcManagedPointersSet.end()) {
                    assert(false);
                }
            }
            for (auto ptr : gcManagedPointersSet) {
                if(managedPointersSet.find(ptr) != managedPointersSet.end()) {
                    assert(false);
                }
            }
        } else if (reader.mode() == '+') {
            ++allocations;
            ++leakedAllocations;
            uint64_t size = 0;
            TraceIndex traceId;
            uint64_t ptr = 0;
            if (!(reader >> size) || !(reader >> traceId.index) || !(reader >> ptr)) {
                cerr << "[W] failed to parse line: " << reader.line() << endl;
                continue;
            }

            AllocationIndex index;
            if (allocationInfos.add(size, traceId, &index, 0)) {
                fprintf(stdout, "a %" PRIx64 " %x 0\n", size, traceId.index);
            }
            ptrToIndex.addPointer(ptr, index);
            lastPtr = ptr;
            fprintf(stdout, "+ %x\n", index.index);
        } else if (reader.mode() == '-') {
            uint64_t ptr = 0;
            if (!(reader >> ptr)) {
                cerr << "[W] failed to parse line: " << reader.line() << endl;
                continue;
            }
            bool temporary = lastPtr == ptr;
            lastPtr = 0;
            auto allocation = ptrToIndex.takePointer(ptr);
            if (!allocation.second) {
                continue;
            }
            fprintf(stdout, "- %x\n", allocation.first.index);
            if (temporary) {
                ++temporaryAllocations;
            }
            --leakedAllocations;
        } else if (reader.mode() == 'n') {
            uint64_t ip;
            string methodOrClassName;
            if (!(reader >> ip) || !(reader >> methodOrClassName)) {
                cerr << "[W] failed to parse line: " << reader.line() << endl;
                continue;
            }

            if (managed_name_ids.find(methodOrClassName) == managed_name_ids.end()) {
                managed_name_ids.insert(std::make_pair(methodOrClassName, 1));
            } else {
                int id = ++managed_name_ids[methodOrClassName];

                methodOrClassName.append("~");
                methodOrClassName.append(std::to_string(id));
            }

            data.addManagedNameForIP(ip, methodOrClassName);
        } else if (reader.mode() == 'e') {
            size_t gcCounter = 0;
            size_t numChildren = 0;
            uintptr_t objectPointer = 0;
            uintptr_t classPointer = 0;

            if (!(reader >> gcCounter) || !(reader >> numChildren) || !(reader >> objectPointer) || !(reader >> classPointer)) {
                cerr << "[W] failed to parse line: " << reader.line() << endl;
                continue;
            }

            // ensure class is encountered
            // TODO: arrays are not reported to the profiler in ClassLoadFinished, so they will be reported in the trace here
            // instead of the next case. We should figure out how to add them to the trace earlier on.
            const auto classId = data.addClass(classPointer);
            if (classId == 0 && classPointer != 0) {
                cerr << "[W] Unknown class id (" << classPointer << ") here: " << reader.line() << endl;
                continue;
            }

            const auto objectId = ptrToIndex.peekPointer(objectPointer);
            if (!objectId.second)
                cerr << "[W] unknown object id (" << objectPointer << ") here: " << reader.line() << endl;
            // trace point, map current output index to parent index
            fprintf(stdout, "e %zx %zx %zx %zx %zx\n", gcCounter, numChildren, objectPointer, classId, objectId.first.index);
        } else if (reader.mode() == 'C') {
            uintptr_t classPointer = 0;
            if (!(reader >> classPointer)) {
                cerr << "[W] failed to parse line: " << reader.line() << endl;
                continue;
            }
            data.addClass(classPointer);
        } else {
            fputs(reader.line().c_str(), stdout);
            fputc('\n', stdout);
        }
    }

    fprintf(stderr, "heaptrack stats:\n"
                    "\tallocations:          \t%" PRIu64 "\n"
                    "\tleaked allocations:   \t%" PRIu64 "\n"
                    "\tmanaged allocations:          \t%" PRIu64 "\n"
                    "\tmanaged leaked allocations:   \t%" PRIu64 "\n"
                    "\ttemporary allocations:\t%" PRIu64 "\n",
            allocations, leakedAllocations, managedAllocations, leakedManagedAllocations, temporaryAllocations);

    return 0;
}
