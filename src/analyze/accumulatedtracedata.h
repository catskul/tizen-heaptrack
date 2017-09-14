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

#ifndef ACCUMULATEDTRACEDATA_H
#define ACCUMULATEDTRACEDATA_H

#include <iosfwd>
#include <tuple>
#include <vector>

#include <fstream>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <boost/functional/hash.hpp>

#include "allocationdata.h"
#include "util/indices.h"

struct Frame
{
    FunctionIndex functionIndex;
    FileIndex fileIndex;
    int line = 0;

    bool operator==(const Frame& rhs) const
    {
        return std::tie(functionIndex, fileIndex, line)
            == std::tie(rhs.functionIndex, rhs.fileIndex, rhs.line);
    }

    bool operator<(const Frame& rhs) const
    {
        return std::tie(functionIndex, fileIndex, line)
             < std::tie(rhs.functionIndex, rhs.fileIndex, rhs.line);
    }
};

struct InstructionPointer
{
    uint64_t instructionPointer = 0;
    ModuleIndex moduleIndex;
    Frame frame;
    std::vector<Frame> inlined;

    bool compareWithoutAddress(const InstructionPointer& other) const
    {
        return std::tie(moduleIndex, frame)
             < std::tie(other.moduleIndex, other.frame);
    }

    bool equalWithoutAddress(const InstructionPointer& other) const
    {
        return std::tie(moduleIndex, frame)
            == std::tie(other.moduleIndex, other.frame);
    }
};

struct TraceNode
{
    IpIndex ipIndex;
    TraceIndex parentIndex;
};

struct Allocation : public AllocationData
{
    // backtrace entry point
    TraceIndex traceIndex;
};

/**
 * Information for a single call to an allocation function.
 */
struct AllocationInfo
{
    uint64_t size = 0;
    TraceIndex traceIndex;
    bool operator==(const AllocationInfo& rhs) const
    {
        return rhs.traceIndex == traceIndex && rhs.size == size;
    }
};

struct AccumulatedTraceData
{
    AccumulatedTraceData();
    virtual ~AccumulatedTraceData() = default;

    virtual void handleTimeStamp(int64_t oldStamp, int64_t newStamp) = 0;
    virtual void handleAllocation(const AllocationInfo& info, const AllocationIndex index) = 0;
    virtual void handleDebuggee(const char* command) = 0;

    const std::string& stringify(const StringIndex stringId) const;

    std::string prettyFunction(const std::string& function) const;

    bool read(const std::string& inputFile);
    enum ParsePass {
        FirstPass,
        SecondPass,
        ThirdPass
    };
    bool read(const std::string& inputFile, const ParsePass pass);
    bool read(std::istream& in, const ParsePass pass);

    void diff(const AccumulatedTraceData& base);

    bool shortenTemplates = false;
    bool fromAttached = false;

    std::vector<Allocation> allocations;
    AllocationData totalCost;
    int64_t totalTime = 0;
    int64_t mallocPeakTime = 0;
    int64_t privateCleanPeakTime = 0;
    int64_t privateDirtyPeakTime = 0;
    int64_t sharedPeakTime = 0;
    int64_t peakRSS = 0;

    struct SystemInfo
    {
        int64_t pages = 0;
        int64_t pageSize = 0;
    };
    SystemInfo systemInfo;

    // our indices are sequentially increasing thus a new allocation can only ever
    // occur with an index larger than any other we encountered so far
    // this can be used to our advantage in speeding up the findAllocation calls.
    TraceIndex m_maxAllocationTraceIndex;

    int64_t getPeakTime()
    {
        switch (AllocationData::display)
        {
            case AllocationData::DisplayId::malloc:
                return mallocPeakTime;
            case AllocationData::DisplayId::privateClean:
                return privateCleanPeakTime;
            case AllocationData::DisplayId::privateDirty:
                return privateDirtyPeakTime;
            default:
                assert(AllocationData::display == AllocationData::DisplayId::shared);
                return sharedPeakTime;
        }
    }

    Allocation& findAllocation(const TraceIndex traceIndex);

    InstructionPointer findIp(const IpIndex ipIndex) const;

    TraceNode findTrace(const TraceIndex traceIndex) const;

    bool isStopIndex(const StringIndex index) const;

    // indices of functions that should stop the backtrace, e.g. main or static
    // initialization
    std::vector<StringIndex> stopIndices;
    std::vector<InstructionPointer> instructionPointers;
    std::vector<TraceNode> traces;
    std::vector<std::string> strings;
    std::vector<IpIndex> opNewIpIndices;

    std::vector<AllocationInfo> allocationInfos;
};

#endif // ACCUMULATEDTRACEDATA_H
