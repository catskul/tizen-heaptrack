/*
 * Copyright 2016-2017 Milian Wolff <mail@milianw.de>
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

#ifndef ALLOCATIONDATA_H
#define ALLOCATIONDATA_H

#include <cassert>
#include <cstdint>

struct AllocationData
{
    enum class DisplayId
    {
        malloc,
        managed,
        privateClean,
        privateDirty,
        shared
    };

    enum class CoreCLRType
    {
        unknown,
        CoreCLR,
        nonCoreCLR,
        untracked
    };

    AllocationData()
        : malloc(),
          managed(),
          privateClean(),
          privateDirty(),
          shared()
    {
    }

    struct Stats
    {
        // number of allocations
        int64_t allocations = 0;
        // number of deallocations
        int64_t deallocations = 0;
        // largest number of instances
        int64_t peak_instances = 0;
        // number of temporary allocations
        int64_t temporary = 0;
        // bytes allocated in total
        int64_t allocated = 0;
        // amount of bytes leaked
        int64_t leaked = 0;
        // largest amount of bytes allocated
        int64_t peak = 0;

        bool isEmpty() const
        {
            return (allocations == 0
                    && deallocations == 0
                    && temporary == 0
                    && allocated == 0
                    && leaked == 0
                    && peak == 0);
        }
    };

    Stats malloc, managed, privateClean, privateDirty, shared;

    static DisplayId display;

    const Stats *getDisplay() const
    {
        switch (display)
        {
            case DisplayId::malloc:
                return &malloc;
            case DisplayId::managed:
                return &managed;
            case DisplayId::privateClean:
                return &privateClean;
            case DisplayId::privateDirty:
                return &privateDirty;
            default:
                assert(display == DisplayId::shared);
                return &shared;
        }
    }
};

inline bool operator==(const AllocationData::Stats &lhs, const AllocationData::Stats &rhs)
{
    return (lhs.allocations == rhs.allocations
            && lhs.deallocations == rhs.deallocations
            && lhs.temporary == rhs.temporary
            && lhs.allocated == rhs.allocated
            && lhs.leaked == rhs.leaked
            && lhs.peak == rhs.peak);
}

inline bool operator!=(const AllocationData::Stats &lhs, const AllocationData::Stats &rhs)
{
    return !(lhs == rhs);
}

inline AllocationData::Stats& operator+=(AllocationData::Stats& lhs, const AllocationData::Stats& rhs)
{
    lhs.allocations += rhs.allocations;
    lhs.deallocations += rhs.deallocations;
    lhs.peak_instances += rhs.peak_instances;
    lhs.temporary += rhs.temporary;
    lhs.allocated += rhs.allocated;
    lhs.leaked += rhs.leaked;
    lhs.peak += rhs.peak;

    return lhs;
}

inline AllocationData::Stats& operator-=(AllocationData::Stats& lhs, const AllocationData::Stats& rhs)
{
    lhs.allocations -= rhs.allocations;
    lhs.deallocations -= rhs.deallocations;
    lhs.peak_instances -= rhs.peak_instances;
    lhs.temporary -= rhs.temporary;
    lhs.allocated -= rhs.allocated;
    lhs.leaked -= rhs.leaked;
    lhs.peak -= rhs.peak;

    return lhs;
}

inline AllocationData::Stats operator+(AllocationData::Stats lhs, const AllocationData::Stats& rhs)
{
    return lhs += rhs;
}

inline AllocationData::Stats operator-(AllocationData::Stats lhs, const AllocationData::Stats& rhs)
{
    return lhs -= rhs;
}

inline bool operator==(const AllocationData& lhs, const AllocationData& rhs)
{
    return (lhs.malloc == rhs.malloc
            && lhs.managed == rhs.managed
            && lhs.privateClean == rhs.privateClean
            && lhs.privateDirty == rhs.privateDirty
            && lhs.shared == rhs.shared);
}

inline bool operator!=(const AllocationData& lhs, const AllocationData& rhs)
{
    return !(lhs == rhs);
}

inline AllocationData& operator+=(AllocationData& lhs, const AllocationData& rhs)
{
    lhs.malloc += rhs.malloc;
    lhs.managed += rhs.managed;
    lhs.privateClean += rhs.privateClean;
    lhs.privateDirty += rhs.privateDirty;
    lhs.shared += rhs.shared;
    return lhs;
}

inline AllocationData& operator-=(AllocationData& lhs, const AllocationData& rhs)
{
    lhs.malloc -= rhs.malloc;
    lhs.managed -= rhs.managed;
    lhs.privateClean -= rhs.privateClean;
    lhs.privateDirty -= rhs.privateDirty;
    lhs.shared -= rhs.shared;
    return lhs;
}

inline AllocationData operator+(AllocationData lhs, const AllocationData& rhs)
{
    return lhs += rhs;
}

inline AllocationData operator-(AllocationData lhs, const AllocationData& rhs)
{
    return lhs -= rhs;
}

#endif // ALLOCATIONDATA_H
