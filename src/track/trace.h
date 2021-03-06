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

#ifndef TRACE_H
#define TRACE_H

#include <cassert>
#include <cstring>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include <execinfo.h>
#include <string>

/**
 * @brief A libunwind based backtrace.
 */
struct Trace
{
    using ip_t = void*;

    enum : int
    {
        MAX_SIZE = 64
    };

    const ip_t* begin() const
    {
        return m_data + m_skip;
    }

    const ip_t* end() const
    {
        return begin() + m_size;
    }

    ip_t operator[](int i) const
    {
        return m_data[m_skip + i];
    }

    int size() const
    {
        return m_size - m_skip;
    }

    __attribute__((noinline))
    bool fill(int skip)
    {
        int size = backtrace(m_data, MAX_SIZE);
        // filter bogus frames at the end, which sometimes get returned by libunwind
        // cf.: https://bugs.kde.org/show_bug.cgi?id=379082
        while (size > 0 && !m_data[size - 1]) {
            --size;
        }
        m_size = size;
        m_skip = skip;
        return m_size > 0;
    }

    void fill(void *addr)
    {
        m_size = 2;
        m_skip = 0;
        m_data[0] = addr;
        m_data[1] = addr;
    }

private:
    int m_size = 0;
    int m_skip = 0;
    ip_t m_data[MAX_SIZE];
};

#endif // TRACE_H
