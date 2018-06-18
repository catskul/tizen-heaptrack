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

#include <stdio.h>

#include <algorithm>
#include <vector>

#ifdef __cplusplus
enum DebugVerbosity
{
    NoDebugOutput,
    MinimalOutput,
    VerboseOutput,
    VeryVerboseOutput,
};

// change this to add more debug output to stderr
constexpr const DebugVerbosity s_debugVerbosity = NoDebugOutput;

/**
 * Call this to optionally show debug information but give the compiler
 * a hand in removing it all if debug output is disabled.
 */
template <DebugVerbosity debugLevel, typename... Args>
inline void debugLog(const char fmt[], Args... args)
{
    if (debugLevel <= s_debugVerbosity) {
        flockfile(stderr);
        fprintf(stderr, "heaptrack debug [%d]: ", static_cast<int>(debugLevel));
        fprintf(stderr, fmt, args...);
        fputc('\n', stderr);
        funlockfile(stderr);
    }
}

/**
 * A per-thread handle guard to prevent infinite recursion, which should be
 * acquired before doing any special symbol handling.
 */
struct RecursionGuard
{
    RecursionGuard()
        : wasLocked(isActive)
    {
        isActive = true;
    }

    ~RecursionGuard()
    {
        isActive = wasLocked;
    }

    const bool wasLocked;
    static thread_local bool isActive;
};

extern "C" {
#endif

typedef void (*heaptrack_callback_t)();
typedef void (*heaptrack_callback_initialized_t)(FILE*);

void heaptrack_init(const char* outputFileName, heaptrack_callback_t initCallbackBefore,
                    heaptrack_callback_initialized_t initCallbackAfter, heaptrack_callback_t stopCallback);

void heaptrack_stop();

void heaptrack_dlopen(const std::vector<std::pair<void *, std::tuple<size_t, int, int>>> &newMmaps, bool isPreloaded, void *dlopenOriginal);
void heaptrack_dlclose(const std::vector<std::pair<void *, size_t>> &unmaps);

void heaptrack_malloc(void* ptr, size_t size);

void heaptrack_free(void* ptr);

void heaptrack_realloc(void* ptr_in, size_t size, void* ptr_out);

void heaptrack_mmap(void* ptr, size_t length, int prot, int flags, int fd, off64_t offset);
void heaptrack_munmap(void* ptr, size_t length);

void heaptrack_invalidate_module_cache();

#ifdef __cplusplus
}
#endif
