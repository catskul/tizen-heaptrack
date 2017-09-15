/*
 * Copyright 2015-2017 Milian Wolff <mail@milianw.de>
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

#include "accumulatedtracedata.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "util/config.h"
#include "util/linereader.h"
#include "util/pointermap.h"

#ifdef __GNUC__
#define POTENTIALLY_UNUSED __attribute__((unused))
#else
#define POTENTIALLY_UNUSED
#endif

using namespace std;

AllocationData::DisplayId AllocationData::display = AllocationData::DisplayId::malloc;

bool AccumulatedTraceData::isHideUnmanagedStackParts = false;
bool AccumulatedTraceData::isShowCoreCLRPartOption = false;

namespace {

template <typename Base>
bool operator>>(LineReader& reader, Index<Base>& index)
{
    return reader.readHex(index.index);
}

template <typename Base>
ostream& operator<<(ostream& out, const Index<Base> index)
{
    out << index.index;
    return out;
}
}

AccumulatedTraceData::AccumulatedTraceData()
{
    instructionPointers.reserve(16384);
    traces.reserve(65536);
    strings.reserve(4096);
    allocations.reserve(16384);
    stopIndices.reserve(4);
    opNewIpIndices.reserve(16);
}

const string& AccumulatedTraceData::stringify(const StringIndex stringId) const
{
    if (!stringId || stringId.index > strings.size()) {
        static const string empty;
        return empty;
    } else {
        return strings.at(stringId.index - 1);
    }
}

string AccumulatedTraceData::prettyFunction(const string& function) const
{
    if (!shortenTemplates) {
        return function;
    }
    string ret;
    ret.reserve(function.size());
    int depth = 0;
    for (size_t i = 0; i < function.size(); ++i) {
        const auto c = function[i];
        if ((c == '<' || c == '>') && ret.size() >= 8) {
            // don't get confused by C++ operators
            const char* cmp = "operator";
            if (ret.back() == c) {
                // skip second angle bracket for operator<< or operator>>
                if (c == '<') {
                    cmp = "operator<";
                } else {
                    cmp = "operator>";
                }
            }
            if (boost::algorithm::ends_with(ret, cmp)) {
                ret.push_back(c);
                continue;
            }
        }
        if (c == '<') {
            ++depth;
            if (depth == 1) {
                ret.push_back(c);
            }
        } else if (c == '>') {
            --depth;
        }
        if (depth) {
            continue;
        }
        ret.push_back(c);
    }
    return ret;
}

bool AccumulatedTraceData::read(const string& inputFile)
{
    if (totalTime == 0) {
        return read(inputFile, FirstPass) && read(inputFile, SecondPass);
    } else {
        return read(inputFile, ThirdPass);
    }
}

bool AccumulatedTraceData::read(const string& inputFile, const ParsePass pass)
{
    const bool isCompressed = boost::algorithm::ends_with(inputFile, ".gz");
    ifstream file(inputFile, isCompressed ? ios_base::in | ios_base::binary : ios_base::in);

    if (!file.is_open()) {
        cerr << "Failed to open heaptrack log file: " << inputFile << endl;
        return false;
    }

    boost::iostreams::filtering_istream in;
    if (isCompressed) {
        in.push(boost::iostreams::gzip_decompressor());
    }
    in.push(file);

    return read(in, pass);
}

bool AccumulatedTraceData::read(istream& in, const ParsePass pass)
{
    LineReader reader;
    int64_t timeStamp = 0;

    vector<string> opNewStrings = {
        // 64 bit
        "operator new(unsigned long)", "operator new[](unsigned long)",
        // 32 bit
        "operator new(unsigned int)", "operator new[](unsigned int)",
    };
    vector<StringIndex> opNewStrIndices;
    opNewStrIndices.reserve(opNewStrings.size());

    vector<string> stopStrings = {"main", "__libc_start_main", "__static_initialization_and_destruction_0"};

    const auto lastMallocPeakCost = pass != FirstPass ? totalCost.malloc.peak : 0;
    const auto lastMallocPeakTime = pass != FirstPass ? mallocPeakTime : 0;

    const auto lastManagedPeakCost = pass != FirstPass ? totalCost.managed.peak : 0;
    const auto lastManagedPeakTime = pass != FirstPass ? managedPeakTime : 0;

    const auto lastPrivateCleanPeakCost = pass != FirstPass ? totalCost.privateClean.peak : 0;
    const auto lastPrivateCleanPeakTime = pass != FirstPass ? privateCleanPeakTime : 0;

    const auto lastPrivateDirtyPeakCost = pass != FirstPass ? totalCost.privateDirty.peak : 0;
    const auto lastPrivateDirtyPeakTime = pass != FirstPass ? privateDirtyPeakTime : 0;

    const auto lastSharedPeakCost = pass != FirstPass ? totalCost.shared.peak : 0;
    const auto lastSharedPeakTime = pass != FirstPass ? sharedPeakTime : 0;

    m_maxAllocationTraceIndex.index = 0;
    totalCost = {};
    mallocPeakTime = 0;
    managedPeakTime = 0;
    privateCleanPeakTime = 0;
    privateDirtyPeakTime = 0;
    sharedPeakTime = 0;
    systemInfo = {};
    peakRSS = 0;
    allocations.clear();
    addressRangeInfos.clear();
    uint fileVersion = 0;
    bool isSmapsChunkInProcess = false;

    // required for backwards compatibility
    // newer versions handle this in heaptrack_interpret already
    AllocationInfoSet allocationInfoSet;
    PointerMap pointers;
    // in older files, this contains the pointer address, in newer formats
    // it holds the allocation info index. both can be used to find temporary
    // allocations, i.e. when a deallocation follows with the same data
    uint64_t lastAllocationPtr = 0;

    while (reader.getLine(in)) {
        if (reader.mode() == 's') {
            if (pass != FirstPass) {
                continue;
            }
            strings.push_back(reader.line().substr(2));
            StringIndex index;
            index.index = strings.size();

            auto opNewIt = find(opNewStrings.begin(), opNewStrings.end(), strings.back());
            if (opNewIt != opNewStrings.end()) {
                opNewStrIndices.push_back(index);
                opNewStrings.erase(opNewIt);
            } else {
                auto stopIt = find(stopStrings.begin(), stopStrings.end(), strings.back());
                if (stopIt != stopStrings.end()) {
                    stopIndices.push_back(index);
                    stopStrings.erase(stopIt);
                }
            }
        } else if (reader.mode() == 't') {
            if (pass != FirstPass) {
                continue;
            }
            TraceNode node;
            reader >> node.ipIndex;
            reader >> node.parentIndex;

            AllocationData::CoreCLRType coreclrType;
            if (isShowCoreCLRPartOption)
            {
                coreclrType = checkIsNodeCoreCLR(node.ipIndex);
            }

            // skip operator new and operator new[] at the beginning of traces
            while (find(opNewIpIndices.begin(), opNewIpIndices.end(), node.ipIndex) != opNewIpIndices.end()) {
                node = findTrace(node.parentIndex);
            }

            if (isHideUnmanagedStackParts) {
                while (node.ipIndex) {
                    const auto& ip = findIp(node.ipIndex);

                    if (ip.isManaged)
                    {
                        break;
                    }

                    node = findTrace(node.parentIndex);
                }
            }

            if (isShowCoreCLRPartOption)
            {
                coreclrType = combineTwoTypes(checkIsNodeCoreCLR(node.ipIndex), coreclrType);
                if (coreclrType != AllocationData::CoreCLRType::CoreCLR)
                {
                    coreclrType = combineTwoTypes(checkCallStackIsCoreCLR(node.parentIndex), coreclrType);
                }
                node.coreclrType = coreclrType;
            }

            traces.push_back(node);
        } else if (reader.mode() == 'i') {
            if (pass != FirstPass) {
                continue;
            }
            InstructionPointer ip;
            reader >> ip.instructionPointer;
            reader >> ip.isManaged;
            reader >> ip.moduleIndex;

            reader >> ip.moduleOffset;

            auto readFrame = [&reader](Frame* frame) {
                return (reader >> frame->functionIndex)
                    && (reader >> frame->fileIndex)
                    && (reader >> frame->line);
            };
            if (readFrame(&ip.frame)) {
                Frame inlinedFrame;
                while (readFrame(&inlinedFrame)) {
                    ip.inlined.push_back(inlinedFrame);
                }
            }

            instructionPointers.push_back(ip);
            if (find(opNewStrIndices.begin(), opNewStrIndices.end(), ip.frame.functionIndex) != opNewStrIndices.end()) {
                IpIndex index;
                index.index = instructionPointers.size();
                opNewIpIndices.push_back(index);
            }
        } else if (reader.mode() == '*') {
            uint64_t length, ptr;
            int prot, fd, isCoreclr;
            TraceIndex traceIndex;

            if (!(reader >> length)
                || !(reader >> prot)
                || !(reader >> isCoreclr)
                || !(reader >> fd)
                || !(reader >> traceIndex.index)
                || !(reader >> ptr)) {
                cerr << "failed to parse line: " << reader.line() << endl;
                continue;
            }

            mapRemoveRanges(ptr, length);

            AddressRangesMapIteratorPair ranges = mapUpdateRange(ptr, length);

            assert (std::next(ranges.first) == ranges.second);

            AddressRangeInfo &rangeInfo = ranges.first->second;

            rangeInfo.setProt(prot);
            rangeInfo.setFd(fd);
            rangeInfo.setIsCoreCLR(isCoreclr);
            rangeInfo.setTraceIndex(traceIndex.index);

            combineContiguousSimilarRanges();

            if (pass != FirstPass) {
                auto& allocation = findAllocation(traceIndex);

                assert(allocation.traceIndex == traceIndex);

                handleTotalCostUpdate();
            }
        } else if (reader.mode() == '/') {
            uint64_t length, ptr;

            if (!(reader >> length)
                || !(reader >> ptr)) {
                cerr << "failed to parse line: " << reader.line() << endl;
                continue;
            }

            mapRemoveRanges(ptr, length);
        } else if (reader.mode() == 'K') {
            int isStart;

            if (!(reader >> isStart) || !(isStart == 1 || isStart == 0)) {
                cerr << "failed to parse line: " << reader.line() << endl;
                continue;
            }

            if (isStart)
            {
                // smaps chunk start
                if (isSmapsChunkInProcess) {
                    cerr << "wrong trace format (nested smaps chunks)" << endl;
                    continue;
                }

                isSmapsChunkInProcess = true;
            }
            else
            {
                assert (!isStart);

                // smaps chunk end
                if (!isSmapsChunkInProcess) {
                    cerr << "wrong trace format (nested smaps chunks?)" << endl;
                    continue;
                }

                combineContiguousSimilarRanges();

                isSmapsChunkInProcess = false;

                totalCost.privateClean.leaked = 0;
                totalCost.privateClean.allocated = 0;

                totalCost.privateDirty.leaked = 0;
                totalCost.privateDirty.allocated = 0;

                totalCost.shared.leaked = 0;
                totalCost.shared.allocated = 0;

                if (pass != FirstPass) {
                    for (auto& allocation : allocations)
                    {
                        allocation.privateClean.leaked = 0;
                        allocation.privateClean.allocated = 0;

                        allocation.privateDirty.leaked = 0;
                        allocation.privateDirty.allocated = 0;

                        allocation.shared.leaked = 0;
                        allocation.shared.allocated = 0;
                    }
                }

                for (auto i = addressRangeInfos.begin (); i != addressRangeInfos.end(); ++i)
                {
                    const AddressRangeInfo& addressRangeInfo = i->second;

                    if(!addressRangeInfo.isPhysicalMemoryConsumptionSet) {
                        cerr << "Unknown range: 0x" << std::hex << addressRangeInfo.start << " (0x" << addressRangeInfo.size << " bytes)" << std::dec << endl;
                        continue;
                    }

                    totalCost.privateClean.allocated += addressRangeInfo.getPrivateClean();
                    totalCost.privateClean.leaked += addressRangeInfo.getPrivateClean();

                    if (totalCost.privateClean.leaked > totalCost.privateClean.peak) {
                        totalCost.privateClean.peak = totalCost.privateClean.leaked;
                        privateCleanPeakTime = timeStamp;

                        if (pass == SecondPass && totalCost.privateClean.peak == lastPrivateCleanPeakCost && privateCleanPeakTime == lastPrivateCleanPeakTime) {
                            for (auto& allocation : allocations) {
                                allocation.privateClean.peak = allocation.privateClean.leaked;
                            }
                        }
                    }

                    totalCost.privateDirty.allocated += addressRangeInfo.getPrivateDirty();
                    totalCost.privateDirty.leaked += addressRangeInfo.getPrivateDirty();

                    if (totalCost.privateDirty.leaked > totalCost.privateDirty.peak) {
                        totalCost.privateDirty.peak = totalCost.privateDirty.leaked;
                        privateDirtyPeakTime = timeStamp;

                        if (pass == SecondPass && totalCost.privateDirty.peak == lastPrivateDirtyPeakCost && privateDirtyPeakTime == lastPrivateDirtyPeakTime) {
                            for (auto& allocation : allocations) {
                                allocation.privateDirty.peak = allocation.privateDirty.leaked;
                            }
                        }
                    }

                    totalCost.shared.allocated += addressRangeInfo.getShared();
                    totalCost.shared.leaked += addressRangeInfo.getShared();

                    if (totalCost.shared.leaked > totalCost.shared.peak) {
                        totalCost.shared.peak = totalCost.shared.leaked;
                        sharedPeakTime = timeStamp;

                        if (pass == SecondPass && totalCost.shared.peak == lastSharedPeakCost && sharedPeakTime == lastSharedPeakTime) {
                            for (auto& allocation : allocations) {
                                allocation.shared.peak = allocation.shared.leaked;
                            }
                        }
                    }

                    if (pass != FirstPass) {
                        auto& allocation = findAllocation(addressRangeInfo.traceIndex);

                        allocation.privateClean.leaked += addressRangeInfo.getPrivateClean();
                        allocation.privateClean.allocated += addressRangeInfo.getPrivateClean();

                        allocation.privateDirty.leaked += addressRangeInfo.getPrivateDirty();
                        allocation.privateDirty.allocated += addressRangeInfo.getPrivateDirty();

                        allocation.shared.leaked += addressRangeInfo.getShared();
                        allocation.shared.allocated += addressRangeInfo.getShared();
                    }
                }

                if (isShowCoreCLRPartOption)
                {
                    if (pass == SecondPass && totalCost.privateClean.peak == lastPrivateCleanPeakCost && timeStamp == lastPrivateCleanPeakTime
                        && AllocationData::display == AllocationData::DisplayId::privateClean)
                    {
                        partCoreclrMMAP.peak = 0;
                        partNonCoreclrMMAP.peak = 0;
                        partUnknownMMAP.peak = 0;
                        partUntrackedMMAP.peak = 0;

                        calculatePeak(AllocationData::DisplayId::privateClean);
                    }

                    if (pass == SecondPass && totalCost.privateDirty.peak == lastPrivateDirtyPeakCost && timeStamp == lastPrivateDirtyPeakTime
                        && AllocationData::display == AllocationData::DisplayId::privateDirty)
                    {
                        partCoreclrMMAP.peak = 0;
                        partNonCoreclrMMAP.peak = 0;
                        partUnknownMMAP.peak = 0;
                        partUntrackedMMAP.peak = 0;

                        calculatePeak(AllocationData::DisplayId::privateDirty);
                    }

                    if (pass == SecondPass && totalCost.shared.peak == lastSharedPeakCost && timeStamp == lastSharedPeakTime
                        && AllocationData::display == AllocationData::DisplayId::shared)
                    {
                        partCoreclrMMAP.peak = 0;
                        partNonCoreclrMMAP.peak = 0;
                        partUnknownMMAP.peak = 0;
                        partUntrackedMMAP.peak = 0;

                        calculatePeak(AllocationData::DisplayId::shared);
                    }
                }
            }

            handleTotalCostUpdate();
        } else if (reader.mode() == 'k') {
            if (!isSmapsChunkInProcess) {
                cerr << "wrong trace format (smaps data outside of smaps chunk)" << endl;
                continue;
            }

            uint64_t addr, diff, size, privateDirty, privateClean, sharedDirty, sharedClean;
            int prot;

            constexpr uint64_t kilobyteSize = 1024;

            if (!(reader >> addr)
                || !(reader >> diff)
                || !(reader >> size)
                || !(reader >> privateDirty)
                || !(reader >> privateClean)
                || !(reader >> sharedDirty)
                || !(reader >> sharedClean)
                || !(reader >> prot)
                /* the value can be different on Tizen for [stack] area */
                /* || !(diff == size * kilobyteSize) */
                ) {
                    cerr << "failed to parse line: " << reader.line() << endl;
                    continue;
            }

            AddressRangesMapIteratorPair ranges = mapUpdateRange(addr, diff);

            for (auto i = ranges.first; i != ranges.second; i++)
            {
                AddressRangeInfo &rangeInfo = i->second;

                rangeInfo.setProt(prot);

                rangeInfo.setPhysicalMemoryConsumption(diff,
                                                       privateDirty * kilobyteSize,
                                                       privateClean * kilobyteSize,
                                                       sharedDirty  * kilobyteSize,
                                                       sharedClean  * kilobyteSize);
            }
        } else if (reader.mode() == '+') {
            AllocationInfo info;
            AllocationIndex allocationIndex;

            if (AllocationData::display != AllocationData::DisplayId::malloc
                && AllocationData::display != AllocationData::DisplayId::managed) {
                // we only need the malloc/calloc/realloc/free details information for malloc and managed statistics
                continue;
            }

            if (fileVersion >= 1) {
                if (!(reader >> allocationIndex.index)) {
                    cerr << "failed to parse line: " << reader.line() << endl;
                    continue;
                } else if (allocationIndex.index >= allocationInfos.size()) {
                    cerr << "allocation index out of bounds: " << allocationIndex.index
                         << ", maximum is: " << allocationInfos.size() << endl;
                    continue;
                }
                info = allocationInfos[allocationIndex.index];
                assert(!info.isManaged);

                lastAllocationPtr = allocationIndex.index;
            } else { // backwards compatibility
                uint64_t ptr = 0;
                if (!(reader >> info.size) || !(reader >> info.traceIndex) || !(reader >> ptr)) {
                    cerr << "failed to parse line: " << reader.line() << endl;
                    continue;
                }
                if (allocationInfoSet.add(info.size, info.traceIndex, &allocationIndex, 0)) {
                    allocationInfos.push_back(info);
                }
                pointers.addPointer(ptr, allocationIndex);
                lastAllocationPtr = ptr;
            }

            if (pass != FirstPass) {
                auto& allocation = findAllocation(info.traceIndex);
                allocation.malloc.leaked += info.size;
                allocation.malloc.allocated += info.size;
                ++allocation.malloc.allocations;

                handleTotalCostUpdate();
                handleAllocation(info, allocationIndex);
            }

            ++totalCost.malloc.allocations;
            totalCost.malloc.allocated += info.size;
            totalCost.malloc.leaked += info.size;
            if (totalCost.malloc.leaked > totalCost.malloc.peak) {
                totalCost.malloc.peak = totalCost.malloc.leaked;
                totalCost.malloc.peak_instances = totalCost.malloc.allocations - totalCost.malloc.deallocations;
                mallocPeakTime = timeStamp;

                if (pass == SecondPass && totalCost.malloc.peak == lastMallocPeakCost && mallocPeakTime == lastMallocPeakTime) {
                    for (auto& allocation : allocations) {
                        allocation.malloc.peak = allocation.malloc.leaked;
                        allocation.malloc.peak_instances = allocation.malloc.allocations - allocation.malloc.deallocations;
                    }
                }
            }
        } else if (reader.mode() == '-') {
            AllocationIndex allocationInfoIndex;
            bool temporary = false;

            if (AllocationData::display != AllocationData::DisplayId::malloc) {
                // we don't need the malloc/calloc/realloc/free details information for malloc statistics
                continue;
            }

            if (fileVersion >= 1) {
                if (!(reader >> allocationInfoIndex.index)) {
                    cerr << "failed to parse line: " << reader.line() << endl;
                    continue;
                }
                temporary = lastAllocationPtr == allocationInfoIndex.index;
            } else { // backwards compatibility
                uint64_t ptr = 0;
                if (!(reader >> ptr)) {
                    cerr << "failed to parse line: " << reader.line() << endl;
                    continue;
                }
                auto taken = pointers.takePointer(ptr);
                if (!taken.second) {
                    // happens when we attached to a running application
                    continue;
                }
                allocationInfoIndex = taken.first;
                temporary = lastAllocationPtr == ptr;
            }
            lastAllocationPtr = 0;

            const auto& info = allocationInfos[allocationInfoIndex.index];
            assert(!info.isManaged);

            totalCost.malloc.leaked -= info.size;
            ++totalCost.malloc.deallocations;
            if (temporary) {
                ++totalCost.malloc.temporary;
            }

            if (pass != FirstPass) {
                auto& allocation = findAllocation(info.traceIndex);
                allocation.malloc.leaked -= info.size;
                ++allocation.malloc.deallocations;
                if (temporary) {
                    ++allocation.malloc.temporary;
                }
            }
        } else if (reader.mode() == '^') {
            AllocationInfo info;
            AllocationIndex allocationIndex;

            if (AllocationData::display != AllocationData::DisplayId::malloc
                && AllocationData::display != AllocationData::DisplayId::managed) {
                // we only need the managed allocation details information for malloc and managed allocation statistics
                continue;
            }

            if (!(reader >> allocationIndex.index)) {
                cerr << "failed to parse line: " << reader.line() << endl;
                continue;
            } else if (allocationIndex.index >= allocationInfos.size()) {
                cerr << "allocation index out of bounds: " << allocationIndex.index
                    << ", maximum is: " << allocationInfos.size() << endl;
                continue;
            }
            info = allocationInfos[allocationIndex.index];

            assert(info.isManaged);

            if (pass != FirstPass) {
                auto& allocation = findAllocation(info.traceIndex);
                allocation.managed.leaked += info.size;
                allocation.managed.allocated += info.size;
                ++allocation.managed.allocations;

                handleTotalCostUpdate();
                handleAllocation(info, allocationIndex);
            }

            ++totalCost.managed.allocations;
            totalCost.managed.allocated += info.size;
            totalCost.managed.leaked += info.size;
            if (totalCost.managed.leaked > totalCost.managed.peak) {
                totalCost.managed.peak = totalCost.managed.leaked;
                totalCost.managed.peak_instances = totalCost.managed.allocations - totalCost.managed.deallocations;
                managedPeakTime = timeStamp;

                if (pass == SecondPass && totalCost.managed.peak == lastManagedPeakCost && managedPeakTime == lastManagedPeakTime) {
                    for (auto& allocation : allocations) {
                        allocation.managed.peak = allocation.managed.leaked;
                        allocation.managed.peak_instances = allocation.managed.allocations - allocation.managed.deallocations;
                    }
                }
            }
        } else if (reader.mode() == '~') {
            AllocationIndex allocationInfoIndex;

            if (AllocationData::display != AllocationData::DisplayId::managed) {
                // we don't need the managed deallocation details information for non-managed allocation statistics
                continue;
            }

            if (!(reader >> allocationInfoIndex.index)) {
                cerr << "failed to parse line: " << reader.line() << endl;
                continue;
            }

            const auto& info = allocationInfos[allocationInfoIndex.index];
            assert(info.isManaged);

            totalCost.managed.leaked -= info.size;
            ++totalCost.managed.deallocations;

            if (pass != FirstPass) {
                auto& allocation = findAllocation(info.traceIndex);
                allocation.managed.leaked -= info.size;
                ++allocation.managed.deallocations;
            }
        } else if (reader.mode() == 'a') {
            if (pass != FirstPass) {
                continue;
            }
            AllocationInfo info;
            if (!(reader >> info.size) || !(reader >> info.traceIndex) || !(reader >> info.isManaged)) {
                cerr << "failed to parse line: " << reader.line() << endl;
                continue;
            }
            allocationInfos.push_back(info);
        } else if (reader.mode() == '#') {
            // comment or empty line
            continue;
        } else if (reader.mode() == 'c') {
            int64_t newStamp = 0;
            if (!(reader >> newStamp)) {
                cerr << "Failed to read time stamp: " << reader.line() << endl;
                continue;
            }
            if (pass != FirstPass) {
                handleTimeStamp(timeStamp, newStamp);
            }
            timeStamp = newStamp;
        } else if (reader.mode() == 'R') { // RSS timestamp
            int64_t rss = 0;
            reader >> rss;
            if (rss > peakRSS) {
                peakRSS = rss;
            }
        } else if (reader.mode() == 'X') {
            if (pass != FirstPass) {
                handleDebuggee(reader.line().c_str() + 2);
            }
        } else if (reader.mode() == 'A') {
            totalCost = {};
            fromAttached = true;
        } else if (reader.mode() == 'v') {
            uint heaptrackVersion = 0;
            reader >> heaptrackVersion;
            if (!(reader >> fileVersion) && heaptrackVersion == 0x010200) {
                // backwards compatibility: before the 1.0.0, I actually
                // bumped the version to 0x010200 already and used that
                // as file version. This is what we now consider v1 of the
                // file format
                fileVersion = 1;
            }
            if (fileVersion > HEAPTRACK_FILE_FORMAT_VERSION) {
                cerr << "The data file has version " << hex << fileVersion << " and was written by heaptrack version "
                     << hex << heaptrackVersion << ")\n"
                     << "This is not compatible with this build of heaptrack (version " << hex << HEAPTRACK_VERSION
                     << "), which can read file format version " << hex << HEAPTRACK_FILE_FORMAT_VERSION << " and below"
                     << endl;
                return false;
            }
        } else if (reader.mode() == 'I') { // system information
            reader >> systemInfo.pageSize;
            reader >> systemInfo.pages;
        } else {
            cerr << "failed to parse line: " << reader.line() << endl;
        }
    }

    if (pass == FirstPass) {
        totalTime = timeStamp + 1;
    } else {
        handleTimeStamp(timeStamp, totalTime);
    }

    if (isShowCoreCLRPartOption)
    {
        AllocationData::Stats totalCoreclr;
        AllocationData::Stats totalNonCoreclr;
        AllocationData::Stats totalUnknown;
        AllocationData::Stats totalUntracked;

        if (AllocationData::display == AllocationData::DisplayId::malloc)
        {
            for (auto it = allocations.begin(); it != allocations.end(); ++it)
            {
                if (!isValidTrace(it->traceIndex))
                {
                    totalUnknown += *(it->getDisplay ());
                    continue;
                }

                AllocationData::CoreCLRType isCoreclr = findTrace(it->traceIndex).coreclrType;

                if (isCoreclr == AllocationData::CoreCLRType::CoreCLR)
                {
                    totalCoreclr += *(it->getDisplay ());
                }
                else if (isCoreclr == AllocationData::CoreCLRType::nonCoreCLR)
                {
                    totalNonCoreclr += *(it->getDisplay ());
                }
                else if (isCoreclr == AllocationData::CoreCLRType::untracked)
                {
                    totalUntracked += *(it->getDisplay ());
                }
                else if (isCoreclr == AllocationData::CoreCLRType::unknown)
                {
                    totalUnknown += *(it->getDisplay ());
                }
            }

            partCoreclr = totalCoreclr;
            partNonCoreclr = totalNonCoreclr;
            partUnknown = totalUnknown;
            partUntracked = totalUntracked;
        }
        else if (AllocationData::display == AllocationData::DisplayId::privateDirty
                 || AllocationData::display == AllocationData::DisplayId::privateClean
                 || AllocationData::display == AllocationData::DisplayId::shared)
        {
            for (auto it = addressRangeInfos.begin(); it != addressRangeInfos.end(); ++it)
            {
                int64_t val = AllocationData::display == AllocationData::DisplayId::privateDirty
                              ? it->second.getPrivateDirty()
                              : AllocationData::display == AllocationData::DisplayId::privateClean
                                ? it->second.getPrivateClean()
                                : AllocationData::display == AllocationData::DisplayId::shared
                                  ? it->second.getShared()
                                  : 0;

                if (!isValidTrace(it->second.traceIndex))
                {
                    totalUnknown.leaked += val;
                    continue;
                }

                AllocationData::CoreCLRType coreclrType = AllocationData::CoreCLRType::unknown;
                if (it->second.isCoreCLR == 0)
                {
                    coreclrType = AllocationData::CoreCLRType::nonCoreCLR;
                }
                else if (it->second.isCoreCLR == 1)
                {
                    coreclrType = AllocationData::CoreCLRType::CoreCLR;
                }
                else if (it->second.isCoreCLR == 2)
                {
                    coreclrType = AllocationData::CoreCLRType::untracked;
                }
                assert(coreclrType != AllocationData::CoreCLRType::unknown);

                AllocationData::CoreCLRType isCoreclr = combineTwoTypes(findTrace(it->second.traceIndex).coreclrType, coreclrType);

                if (isCoreclr == AllocationData::CoreCLRType::CoreCLR)
                {
                    totalCoreclr.leaked += val;
                }
                else if (isCoreclr == AllocationData::CoreCLRType::nonCoreCLR)
                {
                    totalNonCoreclr.leaked += val;
                }
                else if (isCoreclr == AllocationData::CoreCLRType::untracked)
                {
                    totalUntracked.leaked += val;
                }
                else if (isCoreclr == AllocationData::CoreCLRType::unknown)
                {
                    totalUnknown.leaked += val;
                }
            }

            partCoreclrMMAP.leaked = totalCoreclr.leaked;
            partNonCoreclrMMAP.leaked = totalNonCoreclr.leaked;
            partUnknownMMAP.leaked = totalUnknown.leaked;
            partUntrackedMMAP.leaked = totalUntracked.leaked;
        }
    }

    return true;
}

void
AccumulatedTraceData::calculatePeak(AllocationData::DisplayId type)
{
    for (auto i = addressRangeInfos.begin (); i != addressRangeInfos.end(); ++i)
    {
        const AddressRangeInfo& addressRangeInfo = i->second;

        int64_t peak = 0;
        switch (type)
        {
            case AllocationData::DisplayId::privateDirty:
            {
                peak = addressRangeInfo.getPrivateDirty();
                break;
            }
            case AllocationData::DisplayId::privateClean:
            {
                peak = addressRangeInfo.getPrivateClean();
                break;
            }
            case AllocationData::DisplayId::shared:
            {
                peak = addressRangeInfo.getShared();
                break;
            }
            default:
            {
                assert(0);
            }
        }

        if(!addressRangeInfo.isPhysicalMemoryConsumptionSet) {
            continue;
        }

        if (!isValidTrace(addressRangeInfo.traceIndex))
        {
            partUnknownMMAP.peak += peak;
            continue;
        }

        AllocationData::CoreCLRType coreclrType = AllocationData::CoreCLRType::unknown;
        if (addressRangeInfo.isCoreCLR == 0)
        {
            coreclrType = AllocationData::CoreCLRType::nonCoreCLR;
        }
        else if (addressRangeInfo.isCoreCLR == 1)
        {
            coreclrType = AllocationData::CoreCLRType::CoreCLR;
        }
        else if (addressRangeInfo.isCoreCLR == 2)
        {
            coreclrType = AllocationData::CoreCLRType::untracked;
        }

        AllocationData::CoreCLRType isCoreclr = combineTwoTypes(findTrace(addressRangeInfo.traceIndex).coreclrType, coreclrType);

        if (isCoreclr == AllocationData::CoreCLRType::CoreCLR)
        {
            partCoreclrMMAP.peak += peak;
        }
        else if (isCoreclr == AllocationData::CoreCLRType::nonCoreCLR)
        {
            partNonCoreclrMMAP.peak += peak;
        }
        else if (isCoreclr == AllocationData::CoreCLRType::untracked)
        {
            partUntrackedMMAP.peak += peak;
        }
        else if (isCoreclr == AllocationData::CoreCLRType::unknown)
        {
            partUnknownMMAP.peak += peak;
        }
    }
}

AllocationData::CoreCLRType
AccumulatedTraceData::checkIsNodeCoreCLR(IpIndex ipindex)
{
    InstructionPointer ip = findIp(ipindex);
    AllocationData::CoreCLRType coreclrType = AllocationData::CoreCLRType::unknown;

    for (auto iter = addressRangeInfos.begin(); iter != addressRangeInfos.end(); ++iter)
    {
        if (iter->second.start <= ip.instructionPointer && iter->second.start + iter->second.size > ip.instructionPointer)
        {
            if (iter->second.isCoreCLR == 0)
            {
                coreclrType = AllocationData::CoreCLRType::nonCoreCLR;
            }
            else if (iter->second.isCoreCLR == 1)
            {
                coreclrType = AllocationData::CoreCLRType::CoreCLR;
            }
            else if (iter->second.isCoreCLR == 2)
            {
                coreclrType = AllocationData::CoreCLRType::untracked;
            }
            break;
        }
    }
    if (coreclrType == AllocationData::CoreCLRType::unknown)
    {
        // warning here, address is not found in ranges
    }

    return coreclrType;
}

AllocationData::CoreCLRType
AccumulatedTraceData::combineTwoTypes(AllocationData::CoreCLRType a, AllocationData::CoreCLRType b)
{
    if (a == AllocationData::CoreCLRType::CoreCLR
        || b == AllocationData::CoreCLRType::CoreCLR)
    {
        return AllocationData::CoreCLRType::CoreCLR;
    }

    if (a == AllocationData::CoreCLRType::untracked
        || b == AllocationData::CoreCLRType::untracked)
    {
        return AllocationData::CoreCLRType::untracked;
    }

    if (a == AllocationData::CoreCLRType::nonCoreCLR
        || b == AllocationData::CoreCLRType::nonCoreCLR)
    {
        return AllocationData::CoreCLRType::nonCoreCLR;
    }

    return AllocationData::CoreCLRType::unknown;
}

AllocationData::CoreCLRType
AccumulatedTraceData::checkCallStackIsCoreCLR(TraceIndex index)
{
    AllocationData::CoreCLRType isCoreclr = AllocationData::CoreCLRType::unknown;

    while (isValidTrace(index))
    {
        TraceNode node = findTrace(index);
        index = node.parentIndex;

        if (node.coreclrType == AllocationData::CoreCLRType::CoreCLR)
        {
            return node.coreclrType;
        }

        isCoreclr = combineTwoTypes(isCoreclr, node.coreclrType);
    }

    return isCoreclr;
}

namespace { // helpers for diffing

template <typename IndexT, typename SortF>
vector<IndexT> sortedIndices(size_t numIndices, SortF sorter)
{
    vector<IndexT> indices;
    indices.resize(numIndices);
    for (size_t i = 0; i < numIndices; ++i) {
        indices[i].index = (i + 1);
    }
    sort(indices.begin(), indices.end(), sorter);
    return indices;
}

vector<StringIndex> remapStrings(vector<string>& lhs, const vector<string>& rhs)
{
    unordered_map<string, StringIndex> stringRemapping;
    StringIndex stringIndex;
    {
        stringRemapping.reserve(lhs.size());
        for (const auto& string : lhs) {
            ++stringIndex.index;
            stringRemapping.insert(make_pair(string, stringIndex));
        }
    }

    vector<StringIndex> map;
    {
        map.reserve(rhs.size() + 1);
        map.push_back({});
        for (const auto& string : rhs) {
            auto it = stringRemapping.find(string);
            if (it == stringRemapping.end()) {
                ++stringIndex.index;
                lhs.push_back(string);
                map.push_back(stringIndex);
            } else {
                map.push_back(it->second);
            }
        }
    }
    return map;
}

template <typename T>
inline const T& identity(const T& t)
{
    return t;
}

template <typename IpMapper>
int compareTraceIndices(const TraceIndex& lhs, const AccumulatedTraceData& lhsData, const TraceIndex& rhs,
                        const AccumulatedTraceData& rhsData, IpMapper ipMapper)
{
    if (!lhs && !rhs) {
        return 0;
    } else if (lhs && !rhs) {
        return 1;
    } else if (rhs && !lhs) {
        return -1;
    } else if (&lhsData == &rhsData && lhs == rhs) {
        // fast-path if both indices are equal and we compare the same data
        return 0;
    }

    const auto& lhsTrace = lhsData.findTrace(lhs);
    const auto& rhsTrace = rhsData.findTrace(rhs);

    const int parentComparsion =
        compareTraceIndices(lhsTrace.parentIndex, lhsData, rhsTrace.parentIndex, rhsData, ipMapper);
    if (parentComparsion != 0) {
        return parentComparsion;
    } // else fall-through to below, parents are equal

    const auto& lhsIp = lhsData.findIp(lhsTrace.ipIndex);
    const auto& rhsIp = ipMapper(rhsData.findIp(rhsTrace.ipIndex));
    if (lhsIp.equalWithoutAddress(rhsIp)) {
        return 0;
    }
    return lhsIp.compareWithoutAddress(rhsIp) ? -1 : 1;
}

POTENTIALLY_UNUSED void printTrace(const AccumulatedTraceData& data, TraceIndex index)
{
    do {
        const auto trace = data.findTrace(index);
        const auto& ip = data.findIp(trace.ipIndex);
        cerr << index << " (" << trace.ipIndex << ", " << trace.parentIndex << ")" << '\t'
             << data.stringify(ip.frame.functionIndex)
             << " in " << data.stringify(ip.moduleIndex) << "+0x" << std::hex << ip.moduleOffset
             << " at " << data.stringify(ip.frame.fileIndex) << ':' << ip.frame.line << '\n';
        for (const auto& inlined : ip.inlined) {
            cerr << '\t' << data.stringify(inlined.functionIndex) << " at "
                 << data.stringify(inlined.fileIndex) << ':' << inlined.line << '\n';
        }
        index = trace.parentIndex;
    } while (index);
    cerr << "---\n";
}
}

void AccumulatedTraceData::diff(const AccumulatedTraceData& base)
{
    totalCost -= base.totalCost;
    totalTime -= base.totalTime;
    peakRSS -= base.peakRSS;
    systemInfo.pages -= base.systemInfo.pages;
    systemInfo.pageSize -= base.systemInfo.pageSize;

    // step 1: sort trace indices of allocations for efficient lookup
    // step 2: while at it, also merge equal allocations
    vector<TraceIndex> allocationTraceNodes;
    allocationTraceNodes.reserve(allocations.size());
    for (auto it = allocations.begin(); it != allocations.end();) {
        const auto& allocation = *it;
        auto sortedIt =
            lower_bound(allocationTraceNodes.begin(), allocationTraceNodes.end(), allocation.traceIndex,
                        [this](const TraceIndex& lhs, const TraceIndex& rhs) -> bool {
                            return compareTraceIndices(lhs, *this, rhs, *this, identity<InstructionPointer>) < 0;
                        });
        if (sortedIt == allocationTraceNodes.end()
            || compareTraceIndices(allocation.traceIndex, *this, *sortedIt, *this, identity<InstructionPointer>) != 0) {
            allocationTraceNodes.insert(sortedIt, allocation.traceIndex);
            ++it;
        } else if (*sortedIt != allocation.traceIndex) {
            findAllocation(*sortedIt) += allocation;
            it = allocations.erase(it);
        } else {
            ++it;
        }
    }

    // step 3: map string indices from rhs to lhs data

    const auto& stringMap = remapStrings(strings, base.strings);
    auto remapString = [&stringMap](StringIndex& index) {
        if (index) {
            index.index = stringMap[index.index].index;
        }
    };
    auto remapFrame = [&remapString](Frame frame) -> Frame {
        remapString(frame.functionIndex);
        remapString(frame.fileIndex);
        return frame;
    };
    auto remapIp = [&remapString, &remapFrame](InstructionPointer ip) -> InstructionPointer {
        remapString(ip.moduleIndex);
        remapFrame(ip.frame);
        for (auto& inlined : ip.inlined) {
            inlined = remapFrame(inlined);
        }
        return ip;
    };

    // step 4: iterate over rhs data and find matching traces
    //         if no match is found, copy the data over

    auto sortedIps = sortedIndices<IpIndex>(instructionPointers.size(), [this](const IpIndex& lhs, const IpIndex& rhs) {
        return findIp(lhs).compareWithoutAddress(findIp(rhs));
    });

    // map an IpIndex from the rhs data into the lhs data space, or copy the data
    // if it does not exist yet
    auto remapIpIndex = [&sortedIps, this, &base, &remapIp](IpIndex rhsIndex) -> IpIndex {
        if (!rhsIndex) {
            return rhsIndex;
        }

        const auto& rhsIp = base.findIp(rhsIndex);
        const auto& lhsIp = remapIp(rhsIp);

        auto it = lower_bound(sortedIps.begin(), sortedIps.end(), lhsIp,
                              [this](const IpIndex& lhs, const InstructionPointer& rhs) {
                                  return findIp(lhs).compareWithoutAddress(rhs);
                              });
        if (it != sortedIps.end() && findIp(*it).equalWithoutAddress(lhsIp)) {
            return *it;
        }

        instructionPointers.push_back(lhsIp);

        IpIndex ret;
        ret.index = instructionPointers.size();
        sortedIps.insert(it, ret);

        return ret;
    };

    // copy the rhs trace index and the data it references into the lhs data,
    // recursively
    function<TraceIndex(TraceIndex)> copyTrace = [this, &base, remapIpIndex,
                                                  &copyTrace](TraceIndex rhsIndex) -> TraceIndex {
        if (!rhsIndex) {
            return rhsIndex;
        }

        // new location, add it
        const auto& rhsTrace = base.findTrace(rhsIndex);

        TraceNode node;
        node.parentIndex = copyTrace(rhsTrace.parentIndex);
        node.ipIndex = remapIpIndex(rhsTrace.ipIndex);

        traces.push_back(node);
        TraceIndex ret;
        ret.index = traces.size();

        return ret;
    };

    // find an equivalent trace or copy the data over if it does not exist yet
    // a trace is equivalent if the complete backtrace has equal
    // InstructionPointer
    // data while ignoring the actual pointer address
    auto remapTrace = [&base, &allocationTraceNodes, this, remapIp, copyTrace](TraceIndex rhsIndex) -> TraceIndex {
        if (!rhsIndex) {
            return rhsIndex;
        }

        auto it = lower_bound(allocationTraceNodes.begin(), allocationTraceNodes.end(), rhsIndex,
                              [&base, this, remapIp](const TraceIndex& lhs, const TraceIndex& rhs) -> bool {
                                  return compareTraceIndices(lhs, *this, rhs, base, remapIp) < 0;
                              });

        if (it != allocationTraceNodes.end() && compareTraceIndices(*it, *this, rhsIndex, base, remapIp) == 0) {
            return *it;
        }

        TraceIndex ret = copyTrace(rhsIndex);
        allocationTraceNodes.insert(it, ret);
        return ret;
    };

    for (const auto& rhsAllocation : base.allocations) {
        const auto lhsTrace = remapTrace(rhsAllocation.traceIndex);
        assert(remapIp(base.findIp(base.findTrace(rhsAllocation.traceIndex).ipIndex))
                   .equalWithoutAddress(findIp(findTrace(lhsTrace).ipIndex)));
        findAllocation(lhsTrace) -= rhsAllocation;
    }

    // step 5: remove allocations that don't show any differences
    //         note that when there are differences in the backtraces,
    //         we can still end up with merged backtraces that have a total
    //         of 0, but different "tails" of different origin with non-zero cost
    allocations.erase(remove_if(allocations.begin(), allocations.end(),
                                [](const Allocation& allocation) -> bool { return allocation == AllocationData(); }),
                      allocations.end());
}

Allocation& AccumulatedTraceData::findAllocation(const TraceIndex traceIndex)
{
    if (traceIndex < m_maxAllocationTraceIndex) {
        // only need to search when the trace index is previously known
        auto it = lower_bound(allocations.begin(), allocations.end(), traceIndex,
                              [](const Allocation& allocation, const TraceIndex traceIndex) -> bool {
                                  return allocation.traceIndex < traceIndex;
                              });
        if (it == allocations.end() || it->traceIndex != traceIndex) {
            Allocation allocation;
            allocation.traceIndex = traceIndex;
            it = allocations.insert(it, allocation);
        }
        return *it;
    } else if (traceIndex == m_maxAllocationTraceIndex && !allocations.empty()) {
        // reuse the last allocation
        assert(allocations.back().traceIndex == traceIndex);
    } else {
        // actually a new allocation
        Allocation allocation;
        allocation.traceIndex = traceIndex;
        allocations.push_back(allocation);
        m_maxAllocationTraceIndex = traceIndex;
    }
    return allocations.back();
}

InstructionPointer AccumulatedTraceData::findIp(const IpIndex ipIndex) const
{
    if (!ipIndex || ipIndex.index > instructionPointers.size()) {
        return {};
    } else {
        return instructionPointers[ipIndex.index - 1];
    }
}

TraceNode AccumulatedTraceData::findTrace(const TraceIndex traceIndex) const
{
    if (!traceIndex || traceIndex.index > traces.size()) {
        return {};
    } else {
        return traces[traceIndex.index - 1];
    }
}

bool AccumulatedTraceData::isValidTrace(const TraceIndex traceIndex) const
{
    return !(!traceIndex || traceIndex.index > traces.size());
}

TraceNode AccumulatedTraceData::findPrevTrace(const TraceIndex traceIndex) const
{
    TraceNode trace = findTrace(traceIndex);

    return findTrace(trace.parentIndex);
}

bool AccumulatedTraceData::isStopIndex(const StringIndex index) const
{
    return find(stopIndices.begin(), stopIndices.end(), index) != stopIndices.end();
}

AddressRangesMapIteratorPair AccumulatedTraceData::mapUpdateRange(const uint64_t start,
                                                                  const uint64_t size)
{
    uint64_t ptr = start;
    uint64_t endPtr = start + size;

    auto rangesIter = addressRangeInfos.lower_bound(ptr);
    auto rangesEnd = addressRangeInfos.lower_bound(endPtr);

    if (rangesIter != addressRangeInfos.begin())
    {
        auto prev = std::prev(rangesIter);

        if (prev->first + prev->second.size > start)
        {
            AddressRangeInfo newRange = prev->second.split(start - prev->first);

            addressRangeInfos.insert(std::make_pair(start, newRange));

            rangesIter = std::next(prev);

            assert (rangesIter->first == ptr);
        }
    }

    while (ptr != endPtr)
    {
        if (rangesIter == rangesEnd)
        {
            addressRangeInfos.insert(std::make_pair(ptr,
                                                    AddressRangeInfo(ptr,
                                                                     endPtr - ptr)));

            ptr = endPtr;
        }
        else
        {
            if (ptr != rangesIter->first)
            {
                assert(ptr < rangesIter->first);

                uint64_t newSubrangeStart = ptr;
                uint64_t newSubrangeEnd = std::min(endPtr, rangesIter->first);
                uint64_t newSubrangeSize = newSubrangeEnd - newSubrangeStart;

                addressRangeInfos.insert(std::make_pair(ptr,
                                                        AddressRangeInfo(newSubrangeStart,
                                                                         newSubrangeSize)));

                ptr = newSubrangeEnd;
            }
            else
            {
                uint64_t subrangeStart = rangesIter->first;
                uint64_t subrangeEnd = subrangeStart + rangesIter->second.size;

                if (subrangeEnd <= endPtr)
                {
                    ptr = subrangeEnd;

                    ++rangesIter;
                }
                else
                {
                    assert (std::next(rangesIter) == rangesEnd);

                    AddressRangeInfo newRange = rangesIter->second.split(endPtr - ptr);

                    addressRangeInfos.insert(std::make_pair(endPtr, newRange));
                }
            }
        }
    }

    return std::make_pair(addressRangeInfos.lower_bound(start),
                          addressRangeInfos.lower_bound(endPtr));
}

void AccumulatedTraceData::mapRemoveRanges(const uint64_t start,
                                           const uint64_t size)
{
    auto i = addressRangeInfos.lower_bound(start);
    auto e = addressRangeInfos.lower_bound(start + size);

    if (i != addressRangeInfos.begin())
    {
        auto prev = std::prev(i);

        if (prev->first + prev->second.size > start)
        {
            AddressRangeInfo newRange = prev->second.split(start - prev->first);

            addressRangeInfos.insert(std::make_pair(start, newRange));

            i = std::next(prev);

            assert(i->first == start);
        }
    }

    if (i != e)
    {
        auto last = std::prev(e);

        assert (last->first < start + size);

        if (last->first + last->second.size > start + size)
        {
            AddressRangeInfo newRange = last->second.split(start + size - last->first);

            addressRangeInfos.insert(std::make_pair(start + size, newRange));

            e = std::next(last);

            assert(last->first + last->second.size == start + size);
        }
    }

    addressRangeInfos.erase(i, e);
}

void AccumulatedTraceData::combineContiguousSimilarRanges()
{
    for (auto i = addressRangeInfos.begin (); i != addressRangeInfos.end(); ++i)
    {
        assert(i->first == i->second.start);

        auto j = std::next (i);

        assert (j == addressRangeInfos.end()
                || i->second.start + i->second.size <= j->first);

        while (j != addressRangeInfos.end()
               && i->first + i->second.size == j->first)
        {
            if (i->second.combineIfSimilar(j->second))
            {
                addressRangeInfos.erase(j);

                j = std::next (i);
            }
            else
            {
                break;
            }
        }
    }
}
