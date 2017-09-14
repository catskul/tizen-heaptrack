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
 * @file libheaptrack.cpp
 *
 * @brief Collect raw heaptrack data by overloading heap allocation functions.
 */

#include "libheaptrack.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <link.h>
#include <stdio_ext.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <sys/mman.h>

#include <atomic>
#include <cinttypes>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include <boost/algorithm/string/replace.hpp>

#include "tracetree.h"
#include "util/config.h"
#include "util/libunwind_config.h"

/**
 * uncomment this to get extended debug code for known pointers
 * there are still some malloc functions I'm missing apparently,
 * related to TLS and such I guess
 */
// #define DEBUG_MALLOC_PTRS

using namespace std;

thread_local bool RecursionGuard::isActive = false;

namespace {

/**
 * Set to true in an atexit handler. In such conditions, the stop callback
 * will not be called.
 */
atomic<bool> s_atexit{false};

/**
 * Set to true in heaptrack_stop, when s_atexit was not yet set. In such conditions,
 * we always fully unload and cleanup behind ourselves
 */
atomic<bool> s_forceCleanup{false};

void writeVersion(FILE* out)
{
    fprintf(out, "v %x %x\n", HEAPTRACK_VERSION, HEAPTRACK_FILE_FORMAT_VERSION);
}

void writeExe(FILE* out)
{
    const int BUF_SIZE = 1023;
    char buf[BUF_SIZE + 1];
    ssize_t size = readlink("/proc/self/exe", buf, BUF_SIZE);
    if (size > 0 && size < BUF_SIZE) {
        buf[size] = 0;
        fprintf(out, "x %s\n", buf);
    }
}

void writeCommandLine(FILE* out)
{
    fputc('X', out);
    const int BUF_SIZE = 4096;
    char buf[BUF_SIZE + 1];
    auto fd = open("/proc/self/cmdline", O_RDONLY);
    int bytesRead = read(fd, buf, BUF_SIZE);
    char* end = buf + bytesRead;
    for (char* p = buf; p < end;) {
        fputc(' ', out);
        fputs(p, out);
        while (*p++)
            ; // skip until start of next 0-terminated section
    }

    close(fd);
    fputc('\n', out);
}

void writeSystemInfo(FILE* out)
{
    fprintf(out, "I %lx %lx\n", sysconf(_SC_PAGESIZE), sysconf(_SC_PHYS_PAGES));
}

FILE* createFile(const char* fileName)
{
    string outputFileName;
    if (fileName) {
        outputFileName.assign(fileName);
    }

    if (outputFileName == "-" || outputFileName == "stdout") {
        debugLog<VerboseOutput>("%s", "will write to stdout");
        return stdout;
    } else if (outputFileName == "stderr") {
        debugLog<VerboseOutput>("%s", "will write to stderr");
        return stderr;
    }

    if (outputFileName.empty()) {
        // env var might not be set when linked directly into an executable
        outputFileName = "heaptrack.$$";
    }

    boost::replace_all(outputFileName, "$$", to_string(getpid()));

    auto out = fopen(outputFileName.c_str(), "w");
    debugLog<VerboseOutput>("will write to %s/%p\n", outputFileName.c_str(), out);
    // we do our own locking, this speeds up the writing significantly
    __fsetlocking(out, FSETLOCKING_BYCALLER);
    return out;
}

/**
 * Thread-Safe heaptrack API
 *
 * The only critical section in libheaptrack is the output of the data,
 * dl_iterate_phdr
 * calls, as well as initialization and shutdown.
 *
 * This uses a spinlock, instead of a std::mutex, as the latter can lead to
 * deadlocks
 * on destruction. The spinlock is "simple", and OK to only guard the small
 * sections.
 */
class HeapTrack
{
public:
    HeapTrack(const RecursionGuard& /*recursionGuard*/)
        : HeapTrack([] { return true; })
    {
    }

    ~HeapTrack()
    {
        debugLog<VeryVerboseOutput>("%s", "releasing lock");
        s_locked.store(false, memory_order_release);
    }

    void initialize(const char* fileName, heaptrack_callback_t initBeforeCallback,
                    heaptrack_callback_initialized_t initAfterCallback, heaptrack_callback_t stopCallback)
    {
        debugLog<MinimalOutput>("initializing: %s", fileName);
        if (s_data) {
            debugLog<MinimalOutput>("%s", "already initialized");
            return;
        }

        if (initBeforeCallback) {
            debugLog<MinimalOutput>("%s", "calling initBeforeCallback");
            initBeforeCallback();
            debugLog<MinimalOutput>("%s", "done calling initBeforeCallback");
        }

        // do some once-only initializations
        static once_flag once;
        call_once(once, [] {
            debugLog<MinimalOutput>("%s", "doing once-only initialization");
            // configure libunwind for better speed
            if (unw_set_caching_policy(unw_local_addr_space, UNW_CACHE_PER_THREAD)) {
                fprintf(stderr, "WARNING: Failed to enable per-thread libunwind caching.\n");
            }
#ifdef unw_set_cache_size
            if (unw_set_cache_size(unw_local_addr_space, 1024, 0)) {
                fprintf(stderr, "WARNING: Failed to set libunwind cache size.\n");
            }
#endif

            // do not trace forked child processes
            // TODO: make this configurable
            pthread_atfork(&prepare_fork, &parent_fork, &child_fork);

            atexit([]() {
                if (s_forceCleanup) {
                    return;
                }
                debugLog<MinimalOutput>("%s", "atexit()");
                s_atexit.store(true);
                heaptrack_stop();
            });
        });

        FILE* out = createFile(fileName);

        if (!out) {
            fprintf(stderr, "ERROR: Failed to open heaptrack output file: %s\n", fileName);
            if (stopCallback) {
                stopCallback();
            }
            return;
        }

        writeVersion(out);
        writeExe(out);
        writeCommandLine(out);
        writeSystemInfo(out);

        k_pageSize = sysconf(_SC_PAGESIZE);

        s_data = new LockedData(out, stopCallback);

        if (initAfterCallback) {
            debugLog<MinimalOutput>("%s", "calling initAfterCallback");
            initAfterCallback(out);
            debugLog<MinimalOutput>("%s", "calling initAfterCallback done");
        }

        debugLog<MinimalOutput>("%s", "initialization done");
    }

    void shutdown()
    {
        if (!s_data) {
            return;
        }

        debugLog<MinimalOutput>("%s", "shutdown()");

        writeSMAPS();
        writeTimestamp();

        // NOTE: we leak heaptrack data on exit, intentionally
        // This way, we can be sure to get all static deallocations.
        if (!s_atexit || s_forceCleanup) {
            delete s_data;
            s_data = nullptr;
        }

        debugLog<MinimalOutput>("%s", "shutdown() done");
    }

    void invalidateModuleCache()
    {
        if (!s_data) {
            return;
        }
        s_data->moduleCacheDirty = true;
    }

    void writeTimestamp()
    {
        if (!s_data || !s_data->out) {
            return;
        }

        auto elapsed = chrono::duration_cast<chrono::milliseconds>(clock::now() - s_data->start);

        debugLog<VeryVerboseOutput>("writeTimestamp(%" PRIx64 ")", elapsed.count());

        if (fprintf(s_data->out, "c %" PRIx64 "\n", elapsed.count()) < 0) {
            writeError();
            return;
        }
    }

    void writeSMAPS()
    {
        if (!s_data || !s_data->out || !s_data->procSmaps) {
            return;
        }

        if (fprintf(s_data->out, "K 1\n") < 0) {
            writeError();
            return;
        }

        fseek(s_data->procSmaps, 0, SEEK_SET);

        static char smapsLine[2 * PATH_MAX];

        size_t begin = 0, end = 0;
        size_t size, rss, privateDirty, privateClean, sharedDirty, sharedClean;
        size_t totalRSS = 0;
        char protR, protW, protX;
        unsigned int counter = 0;

        while (fgets(smapsLine, sizeof (smapsLine), s_data->procSmaps))
        {
            if (sscanf (smapsLine, "%zx-%zx %c%c%c%*c", &begin, &end, &protR, &protW, &protX) == 5)
            {
                counter++;
            }
            else
            {
                if (sscanf (smapsLine, "Size: %zu kB", &size) == 1)
                {
                    counter++;
                }
                else if (sscanf (smapsLine, "Private_Dirty: %zu kB", &privateDirty) == 1)
                {
                    counter++;
                }
                else if (sscanf (smapsLine, "Private_Clean: %zu kB", &privateClean) == 1)
                {
                    counter++;
                }
                else if (sscanf (smapsLine, "Shared_Dirty: %zu kB", &sharedDirty) == 1)
                {
                    counter++;
                }
                else if (sscanf (smapsLine, "Shared_Clean: %zu kB", &sharedClean) == 1)
                {
                    counter++;
                }
                else if (sscanf (smapsLine, "Rss: %zu kB", &rss) == 1)
                {
                    totalRSS += rss;
                }
                else
                {
                    continue;
                }
            }

            if (counter == 6)
            {
                // all data for current range was read, send to trace

                counter = 0;

                int prot = 0;

                if (protR == 'r')
                    prot |= PROT_READ;

                if (protW == 'w')
                    prot |= PROT_WRITE;

                if (protX == 'x')
                    prot |= PROT_EXEC;

                if (fprintf(s_data->out, "k %zx %zx %zx %zx %zx %zx %zx %x\n",
                            begin, end - begin, size, privateDirty, privateClean, sharedDirty, sharedClean, prot) < 0) {
                    writeError();
                    return;
                }
            }
        }

        if (fprintf(s_data->out, "K 0\n") < 0) {
            writeError();
            return;
        }

        if (fprintf(s_data->out, "R %zx\n", totalRSS) < 0) {
            writeError();
            return;
        }
    }

    void handleMalloc(void* ptr, size_t size, const Trace& trace)
    {
        if (!s_data || !s_data->out) {
            return;
        }
        updateModuleCache();
        const auto index = s_data->traceTree.index(trace, s_data->out);

#ifdef DEBUG_MALLOC_PTRS
        auto it = s_data->known.find(ptr);
        assert(it == s_data->known.end());
        s_data->known.insert(ptr);
#endif

        if (fprintf(s_data->out, "+ %zx %x %" PRIxPTR "\n", size, index, reinterpret_cast<uintptr_t>(ptr)) < 0) {
            writeError();
            return;
        }
    }

    void handleFree(void* ptr)
    {
        if (!s_data || !s_data->out) {
            return;
        }

#ifdef DEBUG_MALLOC_PTRS
        auto it = s_data->known.find(ptr);
        assert(it != s_data->known.end());
        s_data->known.erase(it);
#endif

        if (fprintf(s_data->out, "- %" PRIxPTR "\n", reinterpret_cast<uintptr_t>(ptr)) < 0) {
            writeError();
            return;
        }
    }

    void handleMmap(void* ptr,
                    size_t length,
                    int prot,
                    int fd,
                    const Trace& trace)
    {
        if (!s_data || !s_data->out) {
            return;
        }
        updateModuleCache();
        const auto index = s_data->traceTree.index(trace, s_data->out);

        size_t alignedLength = ((length + k_pageSize - 1) / k_pageSize) * k_pageSize;

        if (fprintf(s_data->out, "* %zx %x %x %x %" PRIxPTR "\n",
                    alignedLength, prot, fd, index, reinterpret_cast<uintptr_t>(ptr)) < 0) {
            writeError();
            return;
        }
    }

    void handleMunmap(void* ptr,
                      size_t length)
    {
        if (!s_data || !s_data->out) {
            return;
        }

        size_t alignedLength = ((length + k_pageSize - 1) / k_pageSize) * k_pageSize;

        if (fprintf(s_data->out, "/ %zx %" PRIxPTR "\n", alignedLength, reinterpret_cast<uintptr_t>(ptr)) < 0) {
            writeError();
            return;
        }
    }

private:
    static int dl_iterate_phdr_callback(struct dl_phdr_info* info, size_t /*size*/, void* data)
    {
        auto heaptrack = reinterpret_cast<HeapTrack*>(data);
        const char* fileName = info->dlpi_name;
        if (!fileName || !fileName[0]) {
            fileName = "x";
        }

        debugLog<VerboseOutput>("dlopen_notify_callback: %s %zx", fileName, info->dlpi_addr);

        const auto MAX_BUILD_ID_SIZE = 20u;
        unsigned raw_build_id_size = 0;
        unsigned char raw_build_id[MAX_BUILD_ID_SIZE] = {};

        for (int i = 0; i < info->dlpi_phnum; i++) {
            const auto& phdr = info->dlpi_phdr[i];
            if (raw_build_id_size == 0 && phdr.p_type == PT_NOTE) {
                auto segmentAddr = phdr.p_vaddr + info->dlpi_addr;
                const auto segmentEnd = segmentAddr + phdr.p_memsz;
                const ElfW(Nhdr)* nhdr = nullptr;
                while (segmentAddr < segmentEnd) {
                    nhdr = reinterpret_cast<ElfW(Nhdr)*>(segmentAddr);
                    if (nhdr->n_type == NT_GNU_BUILD_ID) {
                        break;
                    }
                    segmentAddr += sizeof(ElfW(Nhdr)) + nhdr->n_namesz + nhdr->n_descsz;
                }
                if (nhdr->n_type == NT_GNU_BUILD_ID) {
                    const auto buildIdAddr = segmentAddr + sizeof(ElfW(Nhdr)) + nhdr->n_namesz;
                    if (buildIdAddr + nhdr->n_descsz <= segmentEnd && nhdr->n_descsz <= MAX_BUILD_ID_SIZE) {
                        const auto* buildId = reinterpret_cast<const unsigned char*>(buildIdAddr);
                        raw_build_id_size = nhdr->n_descsz;
                        std::memcpy(raw_build_id, buildId, raw_build_id_size);
                        break;
                    }
                }
            }
        }

        if (fprintf(heaptrack->s_data->out, "m %s ", fileName) < 0) {
            heaptrack->writeError();
            return 1;
        }
        if (raw_build_id_size == 0) {
            if (fprintf(heaptrack->s_data->out, "-------- ") < 0) {
                heaptrack->writeError();
                return 1;
            }
        } else {
            for (unsigned i = 0; i < raw_build_id_size; ++i) {
                if (fprintf(heaptrack->s_data->out, "%02x", raw_build_id[i]) < 0) {
                    heaptrack->writeError();
                    return 1;
                }
            }
        }
        if (fprintf(heaptrack->s_data->out, " %zx", info->dlpi_addr) < 0) {
            heaptrack->writeError();
            return 1;
        }

        for (int i = 0; i < info->dlpi_phnum; i++) {
            const auto& phdr = info->dlpi_phdr[i];
            if (phdr.p_type == PT_LOAD) {
                if (fprintf(heaptrack->s_data->out, " %zx %zx", phdr.p_vaddr, phdr.p_memsz) < 0) {
                    heaptrack->writeError();
                    return 1;
                }
            }
        }

        if (fputc('\n', heaptrack->s_data->out) == EOF) {
            heaptrack->writeError();
            return 1;
        }

        return 0;
    }

    static void prepare_fork()
    {
        debugLog<MinimalOutput>("%s", "prepare_fork()");
        // don't do any custom malloc handling while inside fork
        RecursionGuard::isActive = true;
    }

    static void parent_fork()
    {
        debugLog<MinimalOutput>("%s", "parent_fork()");
        // the parent process can now continue its custom malloc tracking
        RecursionGuard::isActive = false;
    }

    static void child_fork()
    {
        debugLog<MinimalOutput>("%s", "child_fork()");
        // but the forked child process cleans up itself
        // this is important to prevent two processes writing to the same file
        s_data = nullptr;
        RecursionGuard::isActive = true;
    }

    void updateModuleCache()
    {
        if (!s_data || !s_data->out || !s_data->moduleCacheDirty) {
            return;
        }
        debugLog<MinimalOutput>("%s", "updateModuleCache()");
        if (fputs("m -\n", s_data->out) == EOF) {
            writeError();
            return;
        }
        dl_iterate_phdr(&dl_iterate_phdr_callback, this);
        s_data->moduleCacheDirty = false;
    }

    void writeError()
    {
        debugLog<MinimalOutput>("write error %d/%s", errno, strerror(errno));
        s_data->out = nullptr;
        shutdown();
    }

    template <typename AdditionalLockCheck>
    HeapTrack(AdditionalLockCheck lockCheck)
    {
        debugLog<VeryVerboseOutput>("%s", "acquiring lock");
        while (s_locked.exchange(true, memory_order_acquire) && lockCheck()) {
            this_thread::sleep_for(chrono::microseconds(1));
        }
        debugLog<VeryVerboseOutput>("%s", "lock acquired");
    }

    using clock = chrono::steady_clock;

    struct LockedData
    {
        LockedData(FILE* out, heaptrack_callback_t stopCallback)
            : out(out)
            , stopCallback(stopCallback)
        {
            debugLog<MinimalOutput>("%s", "constructing LockedData");

            procSmaps = fopen("/proc/self/smaps", "r");
            if (!procSmaps) {
                fprintf(stderr, "WARNING: Failed to open /proc/self/smaps for reading.\n");
            }

            // ensure this utility thread is not handling any signals
            // our host application may assume only one specific thread
            // will handle the threads, if that's not the case things
            // seemingly break in non-obvious ways.
            // see also: https://bugs.kde.org/show_bug.cgi?id=378494
            sigset_t previousMask;
            sigset_t newMask;
            sigfillset(&newMask);
            if (pthread_sigmask(SIG_SETMASK, &newMask, &previousMask) != 0) {
                fprintf(stderr, "WARNING: Failed to block signals, disabling timer thread.\n");
                return;
            }

            // the mask we set above will be inherited by the thread that we spawn below
            timerThread = thread([&]() {
                RecursionGuard::isActive = true;
                debugLog<MinimalOutput>("%s", "timer thread started");

                int counter = 0;

                // now loop and repeatedly print the timestamp and RSS usage to the data stream
                while (!stopTimerThread) {
                    // TODO: make interval customizable
                    this_thread::sleep_for(chrono::milliseconds(10));

                    HeapTrack heaptrack([&] { return !stopTimerThread.load(); });
                    if (!stopTimerThread) {

                        if (++counter == 32) {
                            heaptrack.writeSMAPS();

                            counter = 0;
                        }
                        heaptrack.writeTimestamp();
                    }
                }
            });

            // now restore the previous mask as if nothing ever happened
            if (pthread_sigmask(SIG_SETMASK, &previousMask, nullptr) != 0) {
                fprintf(stderr, "WARNING: Failed to restore the signal mask.\n");
            }
        }

        ~LockedData()
        {
            debugLog<MinimalOutput>("%s", "destroying LockedData");
            stopTimerThread = true;
            if (timerThread.joinable()) {
                try {
                    timerThread.join();
                } catch (std::system_error) {
                }
            }

            if (out) {
                fclose(out);
            }

            if (procSmaps) {
                fclose(procSmaps);
            }

            if (stopCallback && (!s_atexit || s_forceCleanup)) {
                stopCallback();
            }
            debugLog<MinimalOutput>("%s", "done destroying LockedData");
        }

        /**
         * Note: We use the C stdio API here for performance reasons.
         *       Esp. in multi-threaded environments this is much faster
         *       to produce non-per-line-interleaved output.
         */
        FILE* out = nullptr;

        /// /proc/self/smaps file stream to read address range data from
        FILE* procSmaps = nullptr;

        /**
         * Calls to dlopen/dlclose mark the cache as dirty.
         * When this happened, all modules and their section addresses
         * must be found again via dl_iterate_phdr before we output the
         * next instruction pointer. Otherwise, heaptrack_interpret might
         * encounter IPs of an unknown/invalid module.
         */
        bool moduleCacheDirty = true;

        TraceTree traceTree;

        const chrono::time_point<clock> start = clock::now();
        atomic<bool> stopTimerThread{false};
        thread timerThread;

        heaptrack_callback_t stopCallback = nullptr;

#ifdef DEBUG_MALLOC_PTRS
        unordered_set<void*> known;
#endif
    };

    static atomic<bool> s_locked;
    static LockedData* s_data;

    static size_t k_pageSize;
};

atomic<bool> HeapTrack::s_locked{false};
HeapTrack::LockedData* HeapTrack::s_data{nullptr};
size_t HeapTrack::k_pageSize{0u};
}
extern "C" {

void heaptrack_init(const char* outputFileName, heaptrack_callback_t initBeforeCallback,
                    heaptrack_callback_initialized_t initAfterCallback, heaptrack_callback_t stopCallback)
{
    RecursionGuard guard;

    debugLog<MinimalOutput>("heaptrack_init(%s)", outputFileName);

    HeapTrack heaptrack(guard);
    heaptrack.initialize(outputFileName, initBeforeCallback, initAfterCallback, stopCallback);

    heaptrack.writeSMAPS();
}

void heaptrack_stop()
{
    RecursionGuard guard;

    debugLog<MinimalOutput>("%s", "heaptrack_stop()");

    HeapTrack heaptrack(guard);

    if (!s_atexit) {
        s_forceCleanup.store(true);
    }

    heaptrack.shutdown();
}

void heaptrack_dlopen(const vector<pair<void *, pair<size_t, int>>> &newMmaps, bool isPreloaded, void *dlopenOriginal)
{
    if (!RecursionGuard::isActive) {
        RecursionGuard guard;

        debugLog<VeryVerboseOutput>("heaptrack_dlopen(... [%d] ...)", newMmaps.size());

        Trace trace;

        if (isPreloaded)
        {
            trace.fill(dlopenOriginal);
        } else {
            trace.fill(2);
        }

        HeapTrack heaptrack(guard);

        for (const auto &mmapRecord : newMmaps) {
            heaptrack.handleMmap(mmapRecord.first, mmapRecord.second.first, mmapRecord.second.second, -2 /* FIXME: */, trace);
        }
    }
}

void heaptrack_dlclose(const vector<pair<void *, size_t>> &unmaps)
{
    if (!RecursionGuard::isActive) {
        RecursionGuard guard;

        debugLog<VeryVerboseOutput>("heaptrack_dlclose(... [%d] ...)", unmaps.size());

        HeapTrack heaptrack(guard);
        for (const auto &unmapRecord : unmaps) {
            heaptrack.handleMunmap(unmapRecord.first, unmapRecord.second);
        }
    }
}

void heaptrack_malloc(void* ptr, size_t size)
{
    if (ptr && !RecursionGuard::isActive) {
        RecursionGuard guard;

        debugLog<VeryVerboseOutput>("heaptrack_malloc(%p, %zu)", ptr, size);

        Trace trace;
        trace.fill(2 + HEAPTRACK_DEBUG_BUILD);

        HeapTrack heaptrack(guard);
        heaptrack.handleMalloc(ptr, size, trace);
    }
}

void heaptrack_free(void* ptr)
{
    if (ptr && !RecursionGuard::isActive) {
        RecursionGuard guard;

        debugLog<VeryVerboseOutput>("heaptrack_free(%p)", ptr);

        HeapTrack heaptrack(guard);
        heaptrack.handleFree(ptr);
    }
}

void heaptrack_realloc(void* ptr_in, size_t size, void* ptr_out)
{
    if (ptr_out && !RecursionGuard::isActive) {
        RecursionGuard guard;

        debugLog<VeryVerboseOutput>("heaptrack_realloc(%p, %zu, %p)", ptr_in, size, ptr_out);

        Trace trace;
        trace.fill(2 + HEAPTRACK_DEBUG_BUILD);

        HeapTrack heaptrack(guard);
        if (ptr_in) {
            heaptrack.handleFree(ptr_in);
        }
        heaptrack.handleMalloc(ptr_out, size, trace);
    }
}

void heaptrack_mmap(void* ptr, size_t length, int prot, int flags, int fd, off64_t offset)
{
    if (ptr && !RecursionGuard::isActive) {
        RecursionGuard guard;

        debugLog<VeryVerboseOutput>("heaptrack_mmap(%p, %zu, %d, %d, %d, %llu)",
                                    ptr, length, prot, flags, fd, offset);

        Trace trace;
        trace.fill(2 + HEAPTRACK_DEBUG_BUILD);

        HeapTrack heaptrack(guard);
        heaptrack.handleMmap(ptr, length, prot, fd, trace);
    }
}

void heaptrack_munmap(void* ptr, size_t length)
{
    if (ptr && !RecursionGuard::isActive) {
        RecursionGuard guard;

        debugLog<VeryVerboseOutput>("heaptrack_munmap(%p, %zu)",
                                    ptr, length);

        HeapTrack heaptrack(guard);
        heaptrack.handleMunmap(ptr, length);
    }
}

void heaptrack_invalidate_module_cache()
{
    RecursionGuard guard;

    debugLog<VerboseOutput>("%s", "heaptrack_invalidate_module_cache()");

    HeapTrack heaptrack(guard);
    heaptrack.invalidateModuleCache();
}
}
