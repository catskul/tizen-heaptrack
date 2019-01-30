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

#ifndef TRACETREE_H
#define TRACETREE_H

/**
 * @file tracetree.h
 * @brief Efficiently combine and store the data of multiple Traces.
 */

#include <string.h>

#include <algorithm>
#include <vector>
#include <unordered_set>

#include "trace.h"
#include "outstream/outstream.h"

#include "../profiler/src/stackentry.h"

extern __thread StackEntry* g_shadowStack;

struct TraceEdge
{
    Trace::ip_t instructionPointer;
    // index associated to the backtrace up to this instruction pointer
    // the evaluation process can then reverse-map the index to the parent ip
    // to rebuild the backtrace from the bottom-up
    uint32_t index;
    // sorted list of children, assumed to be small
    std::vector<TraceEdge> children;
};

/**
 * Top-down tree of backtrace instruction pointers.
 *
 * This is supposed to be a memory efficient storage of all instruction pointers
 * ever encountered in any backtrace.
 */
class TraceTree
{
public:
    void clear()
    {
        m_root.children.clear();
        m_index = 1;
    }

    /**
     * Index the data in @p trace and return the index of the last instruction
     * pointer.
     *
     * Unknown instruction pointers will be printed to @p out.
     */
    uint32_t index(const Trace& trace, outStream* out)
    {
        uint32_t index = 0;
        TraceEdge* parent = &m_root;

        auto handleIP = [this, out, &parent, &index] (void *ip, bool isManaged) {
            auto it =
                std::lower_bound(parent->children.begin(), parent->children.end(), ip,
                                 [](const TraceEdge& l, const Trace::ip_t ip) { return l.instructionPointer < ip; });
            if (it == parent->children.end() || it->instructionPointer != ip) {
                index = m_index++;
                it = parent->children.insert(it, {ip, index, {}});
                fprintf(out, "t %" PRIxPTR " %x %x\n", reinterpret_cast<uintptr_t>(ip), parent->index, isManaged ? 1 : 0);
            }
            index = it->index;
            parent = &(*it);
        };

        // process managed stack
        StackEntry *stackIter = g_shadowStack;

        if (stackIter != nullptr) {
            void* managedStack[MANAGED_MAX_SIZE];
            int managedStackSize = 0;

            handleIP((void *) (uintptr_t) -1, false);

            while (stackIter != nullptr && managedStackSize < MANAGED_MAX_SIZE) {
                void *ip = reinterpret_cast<void *>(stackIter->m_funcId);

                if (knownNames.find(ip) == knownNames.end()) {
                    std::string managed_name;
                    if (stackIter->m_isType)
                    {
                        managed_name.append("[");
                        managed_name.append(stackIter->m_className);
                        managed_name.append("]");
                    }
                    else
                    {
                        managed_name.append(stackIter->m_className);
                        managed_name.append(".");
                        managed_name.append(stackIter->m_methodName);
                    }
                    fprintf(out, "n %" PRIxPTR " %s\n", reinterpret_cast<uintptr_t>(ip), managed_name.c_str());
                    knownNames.insert(ip);
                }

                managedStack[managedStackSize++] = ip;

                stackIter = stackIter->m_next;
            }

            for (int i = managedStackSize - 1; i >= 0; --i) {
                handleIP(managedStack[i], true);
            }
        }

        // process unmanaged stack
        for (int i = trace.size() - 1; i >= 0; --i) {
            const auto ip = trace[i];
            if (!ip) {
                continue;
            }

            handleIP(ip, false);
        }
        return index;
    }

    static std::unordered_set<Trace::ip_t> knownNames;

private:
    TraceEdge m_root = {0, 0, {}};
    uint32_t m_index = 1;
    enum : int
    {
        MANAGED_MAX_SIZE = 64
    };
};

#endif // TRACETREE_H
