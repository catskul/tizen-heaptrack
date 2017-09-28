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

#include "parser.h"

#include <KLocalizedString>
#include <ThreadWeaver/ThreadWeaver>

#include <QDebug>

#include "analyze/accumulatedtracedata.h"

#include <future>
#include <tuple>
#include <vector>

using namespace std;

namespace {

struct ObjectNode {
    ObjectNode()
    : m_children(), m_objectPtr(0), m_classIndex(), m_objectSize(0), gcNum(0)
    {
    }

    std::vector<ObjectNode> m_children;
    uint64_t m_objectPtr;
    ClassIndex m_classIndex;
    uint64_t m_objectSize;
    uint32_t gcNum;
};


struct TypeTree {
    TypeTree()
    : m_classIndex(),
      m_parents(),
      m_uniqueObjects(),
      m_totalSize(0),
      m_referencedSize(0),
      m_gcNum(0)
    {
    }

    TypeTree(const TypeTree& rhs)
    : m_classIndex(rhs.m_classIndex),
      m_parents(),
      m_uniqueObjects(rhs.m_uniqueObjects),
      m_totalSize(rhs.m_totalSize),
      m_referencedSize(rhs.m_referencedSize),
      m_gcNum(rhs.m_gcNum)
    {
        for (auto& rParent: rhs.m_parents) {
            auto parent = std::unique_ptr<TypeTree>(new TypeTree(*rParent));
            m_parents.push_back(std::move(parent));
        }
    }

    ~TypeTree() {
    }

    static std::unique_ptr<TypeTree> create(ObjectNode& node) {
        auto TT = std::unique_ptr<TypeTree>(new TypeTree());
        TT->m_classIndex = node.m_classIndex;
        TT->m_gcNum = node.gcNum;
        TT->m_uniqueObjects.insert(node.m_objectPtr);
        TT->m_totalSize = node.m_objectSize;
        TT->m_referencedSize = node.m_objectSize;
        return TT;
    }

    static std::vector<std::unique_ptr<TypeTree>> createBottomUp(ObjectNode& node) {
        std::vector<std::unique_ptr<TypeTree>> result;
        if (node.m_classIndex.index == 0 && node.m_children.size() == 0)
            return result;

        if (node.m_children.size() == 0) {
            result.push_back(create(node));
            return result;
        }

        for (ObjectNode& child: node.m_children) {
            auto leaves = TypeTree::createBottomUp(child);
            for (auto& leaf: leaves) {
                auto parent = create(node);
                parent->m_referencedSize = leaf->m_referencedSize;
                TypeTree* childIt = leaf.get();
                while (childIt->m_parents.size() > 0) {
                    assert(childIt->m_parents.size() == 1 && "Invalid number of m_parents");
                    childIt = childIt->m_parents[0].get();
                }
                childIt->m_parents.push_back(std::move(parent));
                result.push_back(std::move(leaf));
                // Add a copy of the parent to the top level
                result.push_back(create(node));
            }
        }

        return result;
    }

    void mergeSubtrees() {
        std::unordered_map<uint64_t,
                std::vector<TypeTree*>> classCounters;

        for (auto& parent: m_parents) {
            classCounters[parent->m_classIndex.index].push_back(parent.get());
        }

        std::vector<std::unique_ptr<TypeTree>> newParents;

        for (auto it = classCounters.begin(),
             end = classCounters.end(); it != end; ++it) {
            auto combinedParent = std::unique_ptr<TypeTree>(new TypeTree());
            combinedParent->m_classIndex.index = it->first;
            auto& subtrees = it->second;
            combinedParent->m_gcNum = m_gcNum;
            for (auto& parent: subtrees) {
                for (auto &parentsParent: parent->m_parents)
                    combinedParent->m_parents.push_back(std::move(parentsParent));
                combinedParent->m_referencedSize += parent->m_referencedSize;
                for (auto& uo: parent->m_uniqueObjects) {
                    auto pib = combinedParent->m_uniqueObjects.insert(uo);
                    if (pib.second)
                        combinedParent->m_totalSize += parent->m_totalSize;
                }

            }
            combinedParent->mergeSubtrees();

            newParents.push_back(std::move(combinedParent));
        }

        m_parents.erase(m_parents.begin(), m_parents.end());
        m_parents = std::move(newParents);
    }

    ClassIndex m_classIndex;
    std::vector<std::unique_ptr<TypeTree>> m_parents;
    std::unordered_set<uint32_t> m_uniqueObjects;
    uint64_t m_totalSize;
    uint64_t m_referencedSize;
    int m_gcNum;
};

// TODO: use QString directly
struct StringCache
{
    QString func(const Frame& frame, bool isUntrackedLocation) const
    {
        if (frame.functionIndex) {
            // TODO: support removal of template arguments
            return stringify(frame.functionIndex);
        } else {
            if (isUntrackedLocation) {
                return untrackedFunctionName();
            } else {
                return unresolvedFunctionName();
            }
        }
    }

    QString file(const Frame& frame) const
    {
        if (frame.fileIndex) {
            return stringify(frame.fileIndex);
        } else {
            return {};
        }
    }

    QString module(const InstructionPointer& ip) const
    {
        QString moduleName = stringify(ip.moduleIndex);

        if (ip.moduleOffset != 0) {
            QString offset;
            offset.setNum(ip.moduleOffset, 16);

            moduleName = moduleName + QLatin1String("+0x") + offset + QLatin1String("");
        }

        return moduleName;
    }

    QString stringify(const StringIndex index) const
    {
        if (!index || index.index > m_strings.size()) {
            return {};
        } else {
            return m_strings.at(index.index - 1);
        }
    }

    LocationData::Ptr location(const IpIndex& index, const InstructionPointer& ip, bool isUntrackedLocation) const
    {
        // first try a fast index-based lookup
        auto& location = m_locationsMap[index];
        if (!location) {
            location = frameLocation(ip.frame, ip, isUntrackedLocation);
        }
        return location;
    }

    LocationData::Ptr frameLocation(const Frame& frame, const InstructionPointer& ip, bool isUntrackedLocation) const
    {
        LocationData::Ptr location;
        // slow-path, look for interned location
        // note that we can get the same location for different IPs
        LocationData data = {func(frame, isUntrackedLocation), file(frame), module(ip), frame.line};
        auto it = lower_bound(m_locations.begin(), m_locations.end(), data);
        if (it != m_locations.end() && **it == data) {
            // we got the location already from a different ip, cache it
            location = *it;
        } else {
            // completely new location, cache it in both containers
            auto interned = make_shared<LocationData>(data);
            m_locations.insert(it, interned);
            location = interned;
        }
        return location;
    }

    void update(const vector<string>& strings)
    {
        transform(strings.begin() + m_strings.size(), strings.end(), back_inserter(m_strings),
                  [](const string& str) { return QString::fromStdString(str); });
    }

    vector<QString> m_strings;
    mutable vector<LocationData::Ptr> m_locations;
    mutable QHash<IpIndex, LocationData::Ptr> m_locationsMap;

    bool diffMode = false;
};

struct ChartMergeData
{
    IpIndex ip;
    bool isUntrackedLocation;
    qint64 consumed;
    qint64 instances;
    qint64 allocations;
    qint64 allocated;
    qint64 temporary;
    bool operator<(const IpIndex rhs) const
    {
        return ip < rhs;
    }
};

const uint64_t MAX_CHART_DATAPOINTS = 500; // TODO: make this configurable via the GUI

struct ParserData final : public AccumulatedTraceData
{
    ParserData()
    {
    }

    void updateStringCache()
    {
        stringCache.update(strings);
    }

    void prepareBuildCharts()
    {
        if (stringCache.diffMode) {
            return;
        }
        consumedChartData.rows.reserve(MAX_CHART_DATAPOINTS);
        instancesChartData.rows.reserve(MAX_CHART_DATAPOINTS);
        allocatedChartData.rows.reserve(MAX_CHART_DATAPOINTS);
        allocationsChartData.rows.reserve(MAX_CHART_DATAPOINTS);
        temporaryChartData.rows.reserve(MAX_CHART_DATAPOINTS);
        // start off with null data at the origin
        consumedChartData.rows.push_back({});
        instancesChartData.rows.push_back({});
        allocatedChartData.rows.push_back({});
        allocationsChartData.rows.push_back({});
        temporaryChartData.rows.push_back({});
        // index 0 indicates the total row
        consumedChartData.labels[0] = i18n("total");
        instancesChartData.labels[0] = i18n("total");
        allocatedChartData.labels[0] = i18n("total");
        allocationsChartData.labels[0] = i18n("total");
        temporaryChartData.labels[0] = i18n("total");

        buildCharts = true;
        maxConsumedSinceLastTimeStamp = 0;
        maxInstancesSinceLastTimeStamp = 0;
        vector<ChartMergeData> merged;
        merged.reserve(instructionPointers.size());
        // merge the allocation cost by instruction pointer
        // TODO: aggregate by function instead?
        // TODO: traverse the merged call stack up until the first fork
        for (const auto& alloc : allocations) {
            const auto &trace = findPrevTrace(alloc.traceIndex);
            const auto ip = trace.ipIndex;
            bool isUntrackedLocation = (!alloc.traceIndex);
            auto it = lower_bound(merged.begin(), merged.end(), ip);
            if (it == merged.end() || it->ip != ip) {
                it = merged.insert(it, {ip, isUntrackedLocation, 0, 0, 0, 0, 0});
            }
            it->consumed += alloc.getDisplay()->peak; // we want to track the top peaks in the chart
            it->instances += alloc.getDisplay()->allocations - alloc.getDisplay()->deallocations;
            it->allocated += alloc.getDisplay()->allocated;
            it->allocations += alloc.getDisplay()->allocations;
            it->temporary += alloc.getDisplay()->temporary;
        }
        // find the top hot spots for the individual data members and remember their
        // IP and store the label
        auto findTopChartEntries = [&](qint64 ChartMergeData::*member, int LabelIds::*label, ChartData* data) {
            sort(merged.begin(), merged.end(),
                 [=](const ChartMergeData& left, const ChartMergeData& right) { return left.*member > right.*member; });
            for (size_t i = 0; i < min(size_t(ChartRows::MAX_NUM_COST - 1), merged.size()); ++i) {
                const auto& alloc = merged[i];
                if (!(alloc.*member)) {
                    break;
                }
                const auto ip = alloc.ip;
                (labelIds[ip].*label) = i + 1;
                const auto function = stringCache.func(findIp(ip).frame, alloc.isUntrackedLocation);
                const auto module = stringCache.module(findIp(ip));
                data->labels[i + 1] = i18nc("Function and module, if known", "%1 (%2)", function, module);
            }
        };
        findTopChartEntries(&ChartMergeData::consumed, &LabelIds::consumed, &consumedChartData);
        findTopChartEntries(&ChartMergeData::instances, &LabelIds::instances, &instancesChartData);
        findTopChartEntries(&ChartMergeData::allocated, &LabelIds::allocated, &allocatedChartData);
        findTopChartEntries(&ChartMergeData::allocations, &LabelIds::allocations, &allocationsChartData);
        findTopChartEntries(&ChartMergeData::temporary, &LabelIds::temporary, &temporaryChartData);
    }

    void handleTimeStamp(int64_t /*oldStamp*/, int64_t newStamp)
    {
        if (!buildCharts || stringCache.diffMode) {
            return;
        }
        maxConsumedSinceLastTimeStamp = max(maxConsumedSinceLastTimeStamp, totalCost.getDisplay()->leaked);
        maxInstancesSinceLastTimeStamp = max(maxInstancesSinceLastTimeStamp, totalCost.getDisplay()->allocations - totalCost.getDisplay()->deallocations);
        const int64_t diffBetweenTimeStamps = totalTime / MAX_CHART_DATAPOINTS;
        if (newStamp != totalTime && newStamp - lastTimeStamp < diffBetweenTimeStamps) {
            return;
        }
        const auto nowConsumed = maxConsumedSinceLastTimeStamp;
        maxConsumedSinceLastTimeStamp = 0;

        const auto nowInstances = maxInstancesSinceLastTimeStamp;
        maxInstancesSinceLastTimeStamp = 0;

        lastTimeStamp = newStamp;

        // create the rows
        auto createRow = [](int64_t timeStamp, int64_t totalCost) {
            ChartRows row;
            row.timeStamp = timeStamp;
            row.cost[0] = totalCost;
            return row;
        };
        auto consumed = createRow(newStamp, nowConsumed);
        auto instances = createRow(newStamp, nowInstances);
        auto allocated = createRow(newStamp, totalCost.getDisplay()->allocated);
        auto allocs = createRow(newStamp, totalCost.getDisplay()->allocations);
        auto temporary = createRow(newStamp, totalCost.getDisplay()->temporary);

        // if the cost is non-zero and the ip corresponds to a hotspot function
        // selected in the labels,
        // we add the cost to the rows column
        auto addDataToRow = [](int64_t cost, int labelId, ChartRows* rows) {
            if (!cost || labelId == -1) {
                return;
            }
            rows->cost[labelId] += cost;
        };
        for (const auto& alloc : allocations) {
            const auto ip = findPrevTrace(alloc.traceIndex).ipIndex;
            auto it = labelIds.constFind(ip);
            if (it == labelIds.constEnd()) {
                continue;
            }
            const auto& labelIds = *it;
            addDataToRow(alloc.getDisplay()->leaked, labelIds.consumed, &consumed);
            addDataToRow(alloc.getDisplay()->allocations - alloc.getDisplay()->deallocations, labelIds.instances, &instances);
            addDataToRow(alloc.getDisplay()->allocated, labelIds.allocated, &allocated);
            addDataToRow(alloc.getDisplay()->allocations, labelIds.allocations, &allocs);
            addDataToRow(alloc.getDisplay()->temporary, labelIds.temporary, &temporary);
        }
        // add the rows for this time stamp
        consumedChartData.rows << consumed;
        instancesChartData.rows << instances;
        allocatedChartData.rows << allocated;
        allocationsChartData.rows << allocs;
        temporaryChartData.rows << temporary;
    }

    void handleTotalCostUpdate()
    {
        maxConsumedSinceLastTimeStamp = max(maxConsumedSinceLastTimeStamp, totalCost.getDisplay()->leaked);
        maxInstancesSinceLastTimeStamp = max(maxInstancesSinceLastTimeStamp, totalCost.getDisplay()->allocations - totalCost.getDisplay()->deallocations);
    }

    void handleAllocation(const AllocationInfo& info, const AllocationIndex index)
    {
        if (index.index == allocationInfoCounter.size()) {
            allocationInfoCounter.push_back({info, 1});
        } else {
            ++allocationInfoCounter[index.index].allocations;
        }
    }

    void handleDebuggee(const char* command)
    {
        debuggee = command;
    }

    string debuggee;

    struct CountedAllocationInfo
    {
        AllocationInfo info;
        int64_t allocations;
        bool operator<(const CountedAllocationInfo& rhs) const
        {
            return tie(info.size, allocations) < tie(rhs.info.size, rhs.allocations);
        }
    };
    vector<CountedAllocationInfo> allocationInfoCounter;

    ChartData consumedChartData;
    ChartData instancesChartData;
    ChartData allocationsChartData;
    ChartData allocatedChartData;
    ChartData temporaryChartData;
    // here we store the indices into ChartRows::cost for those IpIndices that
    // are within the top hotspots. This way, we can do one hash lookup in the
    // handleTimeStamp function instead of three when we'd store this data
    // in a per-ChartData hash.
    struct LabelIds
    {
        int consumed = -1;
        int instances = -1;
        int allocations = -1;
        int allocated = -1;
        int temporary = -1;
    };
    QHash<IpIndex, LabelIds> labelIds;
    int64_t maxConsumedSinceLastTimeStamp = 0;
    int64_t maxInstancesSinceLastTimeStamp = 0;
    int64_t lastTimeStamp = 0;

    StringCache stringCache;

    bool buildCharts = false;
};

void setParents(QVector<RowData>& children, const RowData* parent)
{
    for (auto& row : children) {
        row.parent = parent;
        setParents(row.children, &row);
    }
}

TreeData mergeAllocations(const ParserData& data, bool bIncludeLeaves)
{
    TreeData topRows;
    auto addRow = [](TreeData* rows, const LocationData::Ptr& location, const Allocation::Stats& cost) -> TreeData* {
        auto it = lower_bound(rows->begin(), rows->end(), location);
        if (it != rows->end() && it->location == location) {
            it->cost += cost;
        } else {
            it = rows->insert(it, {cost, location, nullptr, {}});
        }
        return &it->children;
    };
    // merge allocations, leave parent pointers invalid (their location may change)
    for (const auto& allocation : data.allocations) {
        const AllocationData::Stats *stats = allocation.getDisplay();

        if (stats->isEmpty()) {
            continue;
        }

        auto traceIndex = allocation.traceIndex;

        if (!bIncludeLeaves) {
            traceIndex = data.findTrace(traceIndex).parentIndex;
        }

        auto rows = &topRows;
        do {
            bool isUntrackedLocation = (!traceIndex);

            const auto& trace = data.findTrace(traceIndex);
            const auto& ip = data.findIp(trace.ipIndex);
            if (!(AccumulatedTraceData::isHideUnmanagedStackParts && !ip.isManaged)) {
                auto location = data.stringCache.location(trace.ipIndex, ip, isUntrackedLocation);
                rows = addRow(rows, location, *stats);
                for (const auto& inlined : ip.inlined) {
                    auto inlinedLocation = data.stringCache.frameLocation(inlined, ip, isUntrackedLocation);
                    rows = addRow(rows, inlinedLocation, *stats);
                }
	    }
            if (data.isStopIndex(ip.frame.functionIndex)) {
                break;
            }
            traceIndex = trace.parentIndex;
        } while (traceIndex);
    }
    // now set the parents, the data is constant from here on
    setParents(topRows, nullptr);

    return topRows;
}

RowData* findByLocation(const RowData& row, QVector<RowData>* data)
{
    for (int i = 0; i < data->size(); ++i) {
        if (data->at(i).location == row.location) {
            return data->data() + i;
        }
    }
    return nullptr;
}

AllocationData::Stats buildTopDown(const TreeData& bottomUpData, TreeData* topDownData)
{
    AllocationData::Stats totalCost;
    for (const auto& row : bottomUpData) {
        // recurse and find the cost attributed to children
        const auto childCost = buildTopDown(row.children, topDownData);
        if (childCost != row.cost) {
            // this row is (partially) a leaf
            const auto cost = row.cost - childCost;

            // bubble up the parent chain to build a top-down tree
            auto node = &row;
            auto stack = topDownData;
            while (node) {
                auto data = findByLocation(*node, stack);
                if (!data) {
                    // create an empty top-down item for this bottom-up node
                    *stack << RowData{{}, node->location, nullptr, {}};
                    data = &stack->back();
                }
                // always use the leaf node's cost and propagate that one up the chain
                // otherwise we'd count the cost of some nodes multiple times
                data->cost += cost;
                stack = &data->children;
                node = node->parent;
            }
        }
        totalCost += row.cost;
    }
    return totalCost;
}

QVector<RowData> toTopDownData(const QVector<RowData>& bottomUpData)
{
    QVector<RowData> topRows;
    buildTopDown(bottomUpData, &topRows);
    // now set the parents, the data is constant from here on
    setParents(topRows, nullptr);
    return topRows;
}

AllocationData::Stats buildCallerCallee(const TreeData& bottomUpData, CallerCalleeRows* callerCalleeData)
{
    AllocationData::Stats totalCost;
    for (const auto& row : bottomUpData) {
        // recurse to find a leaf
        const auto childCost = buildCallerCallee(row.children, callerCalleeData);
        if (childCost != row.cost) {
            // this row is (partially) a leaf
            const auto cost = row.cost - childCost;

            // leaf node found, bubble up the parent chain to add cost for all frames
            // to the caller/callee data. this is done top-down since we must not count
            // symbols more than once in the caller-callee data
            QSet<LocationData::Ptr> recursionGuard;

            auto node = &row;
            while (node) {
                const auto& location = node->location;
                if (!recursionGuard.contains(location)) { // aggregate caller-callee data
                    auto it = lower_bound(callerCalleeData->begin(), callerCalleeData->end(), location,
                        [](const CallerCalleeData& lhs, const LocationData::Ptr& rhs) { return lhs.location < rhs; });
                    if (it == callerCalleeData->end() || it->location != location) {
                        it = callerCalleeData->insert(it, {{}, {}, location});
                    }
                    it->inclusiveCost += cost;
                    if (!node->parent) {
                        it->selfCost += cost;
                    }
                    recursionGuard.insert(location);
                }
                node = node->parent;
            }
        }
        totalCost += row.cost;
    }
    return totalCost;
}

CallerCalleeRows toCallerCalleeData(const QVector<RowData>& bottomUpData, bool diffMode)
{
    CallerCalleeRows callerCalleeRows;

    buildCallerCallee(bottomUpData, &callerCalleeRows);

    if (diffMode) {
        // remove rows without cost
        callerCalleeRows.erase(remove_if(callerCalleeRows.begin(), callerCalleeRows.end(),
                                         [](const CallerCalleeData& data) -> bool {
                                             return data.inclusiveCost == AllocationData::Stats()
                                                 && data.selfCost == AllocationData::Stats();
                                         }),
                               callerCalleeRows.end());
    }

    return callerCalleeRows;
}

struct MergedHistogramColumnData
{
    LocationData::Ptr location;
    int64_t allocations;
    bool operator<(const LocationData::Ptr& rhs) const
    {
        return location < rhs;
    }
};

HistogramData buildSizeHistogram(ParserData& data)
{
    HistogramData ret;
    if (data.allocationInfoCounter.empty()) {
        return ret;
    }
    sort(data.allocationInfoCounter.begin(), data.allocationInfoCounter.end());
    const auto totalLabel = i18n("total");
    HistogramRow row;
    const pair<uint64_t, QString> buckets[] = {{8, i18n("0B to 8B")},
                                               {16, i18n("9B to 16B")},
                                               {32, i18n("17B to 32B")},
                                               {64, i18n("33B to 64B")},
                                               {128, i18n("65B to 128B")},
                                               {256, i18n("129B to 256B")},
                                               {512, i18n("257B to 512B")},
                                               {1024, i18n("512B to 1KB")},
                                               {numeric_limits<uint64_t>::max(), i18n("more than 1KB")}};
    uint bucketIndex = 0;
    row.size = buckets[bucketIndex].first;
    row.sizeLabel = buckets[bucketIndex].second;
    vector<MergedHistogramColumnData> columnData;
    columnData.reserve(128);
    auto insertColumns = [&]() {
        sort(columnData.begin(), columnData.end(),
             [](const MergedHistogramColumnData& lhs, const MergedHistogramColumnData& rhs) {
                 return lhs.allocations > rhs.allocations;
             });
        // -1 to account for total row
        for (size_t i = 0; i < min(columnData.size(), size_t(HistogramRow::NUM_COLUMNS - 1)); ++i) {
            const auto& column = columnData[i];
            row.columns[i + 1] = {column.allocations, column.location};
        }
    };
    for (const auto& info : data.allocationInfoCounter) {
        if (info.info.size > row.size) {
            insertColumns();
            columnData.clear();
            ret << row;
            ++bucketIndex;
            row.size = buckets[bucketIndex].first;
            row.sizeLabel = buckets[bucketIndex].second;
            row.columns[0] = {info.allocations, {}};
        } else {
            row.columns[0].allocations += info.allocations;
        }
        const auto &trace = data.findPrevTrace(info.info.traceIndex);
        bool isUntrackedLocation = (!info.info.traceIndex);
        const auto ipIndex = trace.ipIndex;
        const auto ip = data.findIp(ipIndex);
        const auto location = data.stringCache.location(ipIndex, ip, isUntrackedLocation);
        auto it = lower_bound(columnData.begin(), columnData.end(), location);
        if (it == columnData.end() || it->location != location) {
            columnData.insert(it, {location, info.allocations});
        } else {
            it->allocations += info.allocations;
        }
    }
    insertColumns();
    ret << row;
    return ret;
}
}

void setObjectParents(QVector<ObjectRowData>& children, const ObjectRowData* parent)
{
    for (auto& row : children) {
        row.parent = parent;
        setObjectParents(row.children, &row);
    }
}

ObjectRowData objectRowDataFromTypeTree(ParserData& data, TypeTree* tree) {
    ObjectRowData rowData;
    rowData.gcNum = tree->m_gcNum;
    rowData.classIndex = tree->m_classIndex.index;
    rowData.className = data.stringify(tree->m_classIndex);
    rowData.allocations = tree->m_uniqueObjects.size();
    rowData.allocated = tree->m_totalSize;
    rowData.referenced = tree->m_referencedSize;
    for (auto& parent: tree->m_parents)
        rowData.children.push_back(objectRowDataFromTypeTree(data, parent.get()));
    return rowData;
}

ObjectNode buildObjectGraph(ParserData& data, size_t &nodeIndex, bool &success) {
    ObjectNode node;
    if (nodeIndex >= data.objectTreeNodes.size()) {
        success = false;
        return node;
    }
    ObjectTreeNode &dataNode = data.objectTreeNodes[nodeIndex];
    node.m_classIndex = dataNode.classIndex;
    node.m_objectPtr = dataNode.objectPtr;
    node.gcNum = dataNode.gcNum;
    if (dataNode.allocIndex.index != 0)
        node.m_objectSize = data.allocationInfos[dataNode.allocIndex.index].size;
    else
        node.m_objectSize = 0;
    nodeIndex++;
    for (size_t i = 0; i < dataNode.numChildren; ++i) {

        // FIXME: this check will not be needed once we can ensure the
        // integrity of the trace.
        // https://github.sec.samsung.net/dotnet/profiler/issues/24
        if (node.gcNum != data.objectTreeNodes[nodeIndex].gcNum) {
            success = false;
            break;
        }
        node.m_children.push_back(buildObjectGraph(data, nodeIndex, success));
    }
    return node;
}

ObjectTreeData buildObjectTree(ParserData& data)
{

    ObjectTreeData ret;
    size_t nodeIndex = 0;
    std::vector<ObjectNode> nodes;
    while (nodeIndex < data.objectTreeNodes.size()) {
        bool success = true;
        ObjectNode node = buildObjectGraph(data, nodeIndex, success);

        // FIXME: this check will not be needed once we can ensure the
        // integrity of the trace.
        // https://github.sec.samsung.net/dotnet/profiler/issues/24
        if (success)
            nodes.push_back(node);
        else {
            qFatal("Heap snapshot data is incomplete");
        }
    }

    for (auto& node: nodes) {
        TypeTree root;
        root.m_gcNum = node.gcNum;
        root.m_parents = TypeTree::createBottomUp(node);
        root.mergeSubtrees();

        for (auto& parent: root.m_parents) {
            ret.push_back(objectRowDataFromTypeTree(data, parent.get()));
        }

    }
    setObjectParents(ret, nullptr);

    return ret;
}

Parser::Parser(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<SummaryData>();
}

Parser::~Parser() = default;

void Parser::parse(const QString& path, const QString& diffBase)
{
    using namespace ThreadWeaver;
    stream() << make_job([this, path, diffBase]() {
        const auto stdPath = path.toStdString();
        auto data = make_shared<ParserData>();
        emit progressMessageAvailable(i18n("parsing data..."));

        if (!diffBase.isEmpty()) {
            ParserData diffData;
            auto readBase =
                async(launch::async, [&diffData, diffBase]() { return diffData.read(diffBase.toStdString()); });
            if (!data->read(stdPath)) {
                emit failedToOpen(path);
                return;
            }
            if (!readBase.get()) {
                emit failedToOpen(diffBase);
                return;
            }
            data->diff(diffData);
            data->stringCache.diffMode = true;
        } else if (!data->read(stdPath)) {
            emit failedToOpen(path);
            return;
        }

        data->updateStringCache();

        AllocationData::Stats *partCoreclr;
        AllocationData::Stats *partNonCoreclr;
        AllocationData::Stats *partUntracked;
        AllocationData::Stats *partUnknown;

        if (AllocationData::display == AllocationData::DisplayId::malloc)
        {
            partCoreclr = &data->partCoreclr;
            partNonCoreclr = &data->partNonCoreclr;
            partUntracked = &data->partUntracked;
            partUnknown = &data->partUnknown;
        }
        else
        {
            partCoreclr = &data->partCoreclrMMAP;
            partNonCoreclr = &data->partNonCoreclrMMAP;
            partUntracked = &data->partUntrackedMMAP;
            partUnknown = &data->partUnknownMMAP;
        }

        emit summaryAvailable({QString::fromStdString(data->debuggee), *data->totalCost.getDisplay(), data->totalTime, data->getPeakTime(),
                               data->peakRSS * 1024,
                               data->systemInfo.pages * data->systemInfo.pageSize, data->fromAttached,
                               *partCoreclr, *partNonCoreclr, *partUntracked, *partUnknown});

        emit progressMessageAvailable(i18n("merging allocations..."));
        // merge allocations before modifying the data again
        const auto mergedAllocations = mergeAllocations(*data, true);
        emit bottomUpDataAvailable(mergedAllocations);

        if (!AccumulatedTraceData::isHideUnmanagedStackParts) {
            emit bottomUpFilterOutLeavesDataAvailable(mergeAllocations(*data, false));
        } else {
            emit bottomUpFilterOutLeavesDataAvailable(mergedAllocations);
        }

        if (AllocationData::display == AllocationData::DisplayId::managed) {
            const auto objectTreeBottomUpData = buildObjectTree(*data);
            emit objectTreeBottomUpDataAvailable(objectTreeBottomUpData);
        }

        // also calculate the size histogram
        emit progressMessageAvailable(i18n("building size histogram..."));
        const auto sizeHistogram = buildSizeHistogram(*data);
        emit sizeHistogramDataAvailable(sizeHistogram);
        // now data can be modified again for the chart data evaluation

        const auto diffMode = data->stringCache.diffMode;
        emit progressMessageAvailable(i18n("building charts..."));
        auto parallel = new Collection;
        *parallel << make_job([this, mergedAllocations]() {
            const auto topDownData = toTopDownData(mergedAllocations);
            emit topDownDataAvailable(topDownData);
        }) << make_job([this, mergedAllocations, diffMode]() {
            const auto callerCalleeData = toCallerCalleeData(mergedAllocations, diffMode);
            emit callerCalleeDataAvailable(callerCalleeData);
        });
        if (!data->stringCache.diffMode) {
            // only build charts when we are not diffing
            *parallel << make_job([this, data, stdPath]() {
                // this mutates data, and thus anything running in parallel must
                // not access data
                data->prepareBuildCharts();
                data->read(stdPath);
                emit consumedChartDataAvailable(data->consumedChartData);
                emit instancesChartDataAvailable(data->instancesChartData);
                emit allocationsChartDataAvailable(data->allocationsChartData);
                emit allocatedChartDataAvailable(data->allocatedChartData);
                emit temporaryChartDataAvailable(data->temporaryChartData);
            });
        }

        auto sequential = new Sequence;
        *sequential << parallel << make_job([this]() { emit finished(); });

        stream() << sequential;
    });
}
