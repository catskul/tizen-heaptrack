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

#include "libheaptrack.h"
#include "util/config.h"

#include <cstdlib>
#include <cstring>

#include <link.h>
#include <malloc.h>

#include <sys/mman.h>

#include <map>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * @file heaptrack_inject.cpp
 *
 * @brief Experimental support for symbol overloading after runtime injection.
 */

#if __WORDSIZE == 64
#define ELF_R_SYM(i) ELF64_R_SYM(i)
#elif __WORDSIZE == 32
#define ELF_R_SYM(i) ELF32_R_SYM(i)
#else
#error unsupported word size
#endif

static int isCoreCLR(const char *filename)
{
    const char *localFilename = strrchr(filename,'/');
    if (!localFilename)
    {
        localFilename = filename;
    }
    else
    {
        ++localFilename;
    }

    if (strcmp(localFilename, "libclrjit.so") == 0
        || strcmp(localFilename, "libcoreclr.so") == 0
        || strcmp(localFilename, "libcoreclrtraceptprovider.so") == 0
        || strcmp(localFilename, "libdbgshim.so") == 0
        || strcmp(localFilename, "libmscordaccore.so") == 0
        || strcmp(localFilename, "libmscordbi.so") == 0
        || strcmp(localFilename, "libprotojit.so") == 0
        || strcmp(localFilename, "libsosplugin.so") == 0
        || strcmp(localFilename, "libsos.so") == 0
        || strcmp(localFilename, "libsuperpmi-shim-collector.so") == 0
        || strcmp(localFilename, "libsuperpmi-shim-counter.so") == 0
        || strcmp(localFilename, "libsuperpmi-shim-simple.so") == 0
        || strcmp(localFilename, "System.Globalization.Native.so") == 0
        || strcmp(localFilename, "System.IO.Compression.Native.so") == 0
        || strcmp(localFilename, "System.Native.so") == 0
        || strcmp(localFilename, "System.Net.Http.Native.so") == 0
        || strcmp(localFilename, "System.Net.Security.Native.so") == 0
        || strcmp(localFilename, "System.Security.Cryptography.Native.OpenSsl.so") == 0
        || strcmp(localFilename, "System.Security.Cryptography.Native.so") == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static bool isExiting = false;

static int dl_iterate_phdr_get_maps(struct dl_phdr_info* info, size_t /*size*/, void* data)
{
    auto maps = (std::map<void *, std::tuple<size_t, int, int>> *) data;

    const char* fileName = info->dlpi_name;
    if (!fileName || !fileName[0]) {
        fileName = "x";
    }

    int isCoreclr = isCoreCLR(fileName);

    debugLog<VerboseOutput>("dl_iterate_phdr_get_maps: %s %zx", fileName, info->dlpi_addr);

    for (int i = 0; i < info->dlpi_phnum; i++) {
        const auto& phdr = info->dlpi_phdr[i];

        if (phdr.p_type == PT_LOAD) {
            constexpr uintptr_t pageMask = (uintptr_t) 0xfff;

            uintptr_t start = (info->dlpi_addr + phdr.p_vaddr) & ~pageMask;
            uintptr_t end = (info->dlpi_addr + phdr.p_vaddr + phdr.p_memsz + pageMask) & ~pageMask;

            void *addr = (void *) start;
            size_t size = (size_t) (end - start);

            int prot = 0;

            if (phdr.p_flags & PF_R)
                prot |= PROT_READ;
            if (phdr.p_flags & PF_W)
                prot |= PROT_WRITE;
            if (phdr.p_flags & PF_X)
                prot |= PROT_EXEC;

            if (maps->find(addr) == maps->end()) {
                maps->insert(std::make_pair(addr, std::make_tuple(size, prot, isCoreclr)));
            } else {
                debugLog<VerboseOutput>("dl_iterate_phdr_get_maps: repeated section address %s %zx", fileName, info->dlpi_addr);
            }
        }
    }

    return 0;
}

namespace {

namespace Elf {
using Addr = ElfW(Addr);
using Dyn = ElfW(Dyn);
using Rel = ElfW(Rel);
using Rela = ElfW(Rela);
using Sym = ElfW(Sym);
using Sxword = ElfW(Sxword);
using Xword = ElfW(Xword);

union Rel_Rela
{
    Rel rel;
    Rela rela;
};
}

void overwrite_symbols() noexcept;

namespace hooks {

struct malloc
{
    static constexpr auto name = "malloc";
    static constexpr auto original = &::malloc;

    static void* hook(size_t size) noexcept
    {
        auto ptr = original(size);
        heaptrack_malloc(ptr, size);
        return ptr;
    }
};

struct free
{
    static constexpr auto name = "free";
    static constexpr auto original = &::free;

    static void hook(void* ptr) noexcept
    {
        heaptrack_free(ptr);
        original(ptr);
    }
};

struct realloc
{
    static constexpr auto name = "realloc";
    static constexpr auto original = &::realloc;

    static void* hook(void* ptr, size_t size) noexcept
    {
        auto ret = original(ptr, size);
        heaptrack_realloc(ptr, size, ret);
        return ret;
    }
};

struct calloc
{
    static constexpr auto name = "calloc";
    static constexpr auto original = &::calloc;

    static void* hook(size_t num, size_t size) noexcept
    {
        auto ptr = original(num, size);
        heaptrack_malloc(ptr, num * size);
        return ptr;
    }
};

#if HAVE_CFREE
struct cfree
{
    static constexpr auto name = "cfree";
    static constexpr auto original = &::cfree;

    static void hook(void* ptr) noexcept
    {
        heaptrack_free(ptr);
        original(ptr);
    }
};
#endif

struct dlopen
{
    static constexpr auto name = "dlopen";
    static constexpr auto original = &::dlopen;

    static void* hook(const char* filename, int flag) noexcept
    {
        std::map<void *, std::tuple<size_t, int, int>> map_before, map_after;

        if (!RecursionGuard::isActive) {
            RecursionGuard guard;
            dl_iterate_phdr(&dl_iterate_phdr_get_maps, &map_before);
        }

        auto ret = original(filename, flag);

        if (ret) {
            heaptrack_invalidate_module_cache();
            overwrite_symbols();

            if (!RecursionGuard::isActive) {
                RecursionGuard guard;
                dl_iterate_phdr(&dl_iterate_phdr_get_maps, &map_after);
            }

            if(map_after.size() < map_before.size()) {
                debugLog<VerboseOutput>("dlopen: count of sections after dlopen is less than before: %p %s %x", ret, filename, flag);
            } else if (map_after.size() != map_before.size()) {
                std::vector<std::pair<void *, std::tuple<size_t, int, int>>> newMmaps;

                if (!RecursionGuard::isActive) {
                    RecursionGuard guard;

                    for (const auto & section_after : map_after) {
                        if (map_before.find(section_after.first) == map_before.end()) {
                            newMmaps.push_back(section_after);
                        }
                    }
                }

                heaptrack_dlopen(newMmaps, false, reinterpret_cast<void *>(hooks::dlopen::original));

                if (!RecursionGuard::isActive) {
                    RecursionGuard guard;

                    newMmaps.clear();
                }
            }
        }
        return ret;
    }
};

struct dlclose
{
    static constexpr auto name = "dlclose";
    static constexpr auto original = &::dlclose;

    static int hook(void* handle) noexcept
    {
        std::map<void *, std::tuple<size_t, int, int>> map_before, map_after;

        if (!isExiting) {
            if (!RecursionGuard::isActive) {
                RecursionGuard guard;
                dl_iterate_phdr(&dl_iterate_phdr_get_maps, &map_before);
            }
        }

        auto ret = original(handle);
        if (!ret) {
            heaptrack_invalidate_module_cache();

            if (!isExiting) {
                if (!RecursionGuard::isActive) {
                    RecursionGuard guard;
                    dl_iterate_phdr(&dl_iterate_phdr_get_maps, &map_after);
                }

                if(map_after.size() > map_before.size()) {
                    debugLog<VerboseOutput>("dlopen: count of sections after dlclose is greater than before: %p", handle);
                } else if (map_after.size() != map_before.size()) {
                    std::vector<std::pair<void *, size_t>> munmaps;

                    if (!RecursionGuard::isActive) {
                        RecursionGuard guard;

                        for (const auto & section_before : map_before) {
                            if (map_after.find(section_before.first) == map_after.end()) {
                                munmaps.push_back(std::make_pair(section_before.first, std::get<0>(section_before.second)));
                            }
                        }
                    }

                    heaptrack_dlclose(munmaps);

                    if (!RecursionGuard::isActive) {
                        RecursionGuard guard;

                        munmaps.clear();
                    }
                }
            }
        }
        return ret;
    }
};

struct posix_memalign
{
    static constexpr auto name = "posix_memalign";
    static constexpr auto original = &::posix_memalign;

    static int hook(void** memptr, size_t alignment, size_t size) noexcept
    {
        auto ret = original(memptr, alignment, size);
        if (!ret) {
            size_t aligned_size = ((size + alignment - 1) / alignment) * alignment;

            heaptrack_malloc(*memptr, aligned_size);
        }
        return ret;
    }
};

struct mmap
{
    static constexpr auto name = "mmap";
    static constexpr auto original = &::mmap;

    static void *hook(void *addr,
                      size_t length,
                      int prot,
                      int flags,
                      int fd,
                      off_t offset) noexcept
    {
        void *ret = original(addr, length, prot, flags, fd, offset);

        if (ret != MAP_FAILED) {
            heaptrack_mmap(ret, length, prot, flags, fd, offset);
        }

        return ret;
    }
};

struct mmap64
{
    static constexpr auto name = "mmap64";
    static constexpr auto original = &::mmap64;

    static void *hook(void *addr,
                      size_t length,
                      int prot,
                      int flags,
                      int fd,
                      off64_t offset) noexcept
    {
        void *ret = original(addr, length, prot, flags, fd, offset);

        if (ret != MAP_FAILED) {
            heaptrack_mmap(ret, length, prot, flags, fd, offset);
        }

        return ret;
    }
};

struct munmap
{
    static constexpr auto name = "munmap";
    static constexpr auto original = &::munmap;

    static int hook(void *addr,
                    size_t length)
    {
        int ret = original(addr, length);

        if (ret != -1) {
            heaptrack_munmap(addr, length);
        }

        return ret;
    }
};

template <typename Hook>
bool hook(const char* symname, Elf::Addr addr, bool restore)
{
    static_assert(std::is_convertible<decltype(&Hook::hook), decltype(Hook::original)>::value,
                  "hook is not compatible to original function");

    if (strcmp(Hook::name, symname) != 0) {
        return false;
    }

    // try to make the page read/write accessible, which is hackish
    // but apparently required for some shared libraries
    auto page = reinterpret_cast<void*>(addr & ~(0x1000 - 1));
    mprotect(page, 0x1000, PROT_READ | PROT_WRITE);

    // now write to the address
    auto typedAddr = reinterpret_cast<typename std::remove_const<decltype(Hook::original)>::type*>(addr);
    if (restore) {
        // restore the original address on shutdown
        *typedAddr = Hook::original;
    } else {
        // now actually inject our hook
        *typedAddr = &Hook::hook;
    }

    return true;
}

void apply(const char* symname, Elf::Addr addr, bool restore)
{
    // TODO: use std::apply once we can rely on C++17
    hook<malloc>(symname, addr, restore) || hook<free>(symname, addr, restore) || hook<realloc>(symname, addr, restore)
        || hook<calloc>(symname, addr, restore)
#if HAVE_CFREE
        || hook<cfree>(symname, addr, restore)
#endif
        || hook<posix_memalign>(symname, addr, restore) || hook<dlopen>(symname, addr, restore)
        || hook<dlclose>(symname, addr, restore)
        || hook<mmap>(symname, addr, restore)
        || hook<mmap64>(symname, addr, restore)
        || hook<munmap>(symname, addr, restore);
}
}

template <typename T, Elf::Sxword AddrTag, Elf::Sxword SizeTag>
struct elftable
{
    T* table = nullptr;
    Elf::Xword size = {};

    bool consume(const Elf::Dyn* dyn) noexcept
    {
        if (dyn->d_tag == AddrTag) {
            table = reinterpret_cast<T*>(dyn->d_un.d_ptr);
            return true;
        } else if (dyn->d_tag == SizeTag) {
            size = dyn->d_un.d_val;
            return true;
        }
        return false;
    }
};

using elf_string_table = elftable<const char, DT_STRTAB, DT_STRSZ>;
using elf_jmprel_table = elftable<Elf::Rel_Rela, DT_JMPREL, DT_PLTRELSZ>;
using elf_symbol_table = elftable<Elf::Sym, DT_SYMTAB, DT_SYMENT>;

template<typename T>
void try_overwrite_symbols_rel_or_rela(const elf_jmprel_table &jmprels,
                                       const elf_string_table &strings,
                                       const elf_symbol_table &symbols,
                                       const Elf::Addr base,
                                       const bool restore)
{
    const auto rel_end = reinterpret_cast<T*>(reinterpret_cast<char*>(jmprels.table) + jmprels.size);
    for (auto rel = reinterpret_cast<T*>(jmprels.table); rel < rel_end; rel++) {
        const auto index = ELF_R_SYM(rel->r_info);
        const char* symname = strings.table + symbols.table[index].st_name;
        auto addr = rel->r_offset + base;

        hooks::apply(symname, addr, restore);
    }
}

void try_overwrite_symbols(const Elf::Dyn* dyn, const Elf::Addr base, const bool restore) noexcept
{
    elf_symbol_table symbols;
    elf_jmprel_table jmprels;
    elf_string_table strings;

    bool is_rela = false;

    // initialize the elf tables
    for (; dyn->d_tag != DT_NULL; ++dyn) {
        if (dyn->d_tag == DT_PLTREL && dyn->d_un.d_val == DT_RELA) {
            is_rela = true;
        }

        symbols.consume(dyn) || jmprels.consume(dyn) || strings.consume(dyn);
    }

    // find symbols to overwrite
    if (is_rela) {
        try_overwrite_symbols_rel_or_rela<Elf::Rela>(jmprels, strings, symbols, base, restore);
    } else {
        try_overwrite_symbols_rel_or_rela<Elf::Rel>(jmprels, strings, symbols, base, restore);
    }
}

int iterate_phdrs(dl_phdr_info* info, size_t /*size*/, void* data) noexcept
{
    if (strstr(info->dlpi_name, "/libheaptrack_inject.so")) {
        // prevent infinite recursion: do not overwrite our own symbols
        return 0;
    } else if (strstr(info->dlpi_name, "/ld-linux")) {
        // prevent strange crashes due to overwriting the free symbol in ld-linux
        return 0;
    }

    for (auto phdr = info->dlpi_phdr, end = phdr + info->dlpi_phnum; phdr != end; ++phdr) {
        if (phdr->p_type == PT_DYNAMIC) {
            try_overwrite_symbols(reinterpret_cast<const Elf::Dyn*>(phdr->p_vaddr + info->dlpi_addr), info->dlpi_addr,
                                  data != nullptr);
        }
    }
    return 0;
}

void overwrite_symbols() noexcept
{
    dl_iterate_phdr(&iterate_phdrs, nullptr);
}
}

extern "C" {

void heaptrack_inject(const char* outputFileName) noexcept
{
    atexit([]() {
        isExiting = true;
    });

    heaptrack_init(outputFileName, []() { overwrite_symbols(); }, [](FILE* out) { fprintf(out, "A\n"); },
                   []() {
                       bool do_shutdown = true;
                       dl_iterate_phdr(&iterate_phdrs, &do_shutdown);
                   });

   {
       std::map<void *, std::tuple<size_t, int, int>> map;

       dl_iterate_phdr(&dl_iterate_phdr_get_maps, &map);

       std::vector<std::pair<void *, std::tuple<size_t, int, int>>> newMmaps;

       for (const auto & section : map) {
           newMmaps.push_back(section);
       }

       heaptrack_dlopen(newMmaps, true, reinterpret_cast<void *>(hooks::dlopen::original));
   }
}

#if TIZEN
int dotnet_launcher_inject() noexcept
{
    int res = -1;
    char *env = nullptr;
    char* output = nullptr;

    env = getenv("DUMP_HEAPTRACK_OUTPUT");
    if (env == nullptr) {
        goto ret;
    }

    output = strdup(env);
    if (output == nullptr) {
        goto ret;
    }

    heaptrack_inject(output);
    res = 0;

ret:
    unsetenv("DUMP_HEAPTRACK_OUTPUT");

    free(output);

    return res;
}
#endif

}
