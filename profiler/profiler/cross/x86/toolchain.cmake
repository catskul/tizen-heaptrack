set(CROSS_ROOTFS $ENV{ROOTFS_DIR})
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

add_compile_options("-m32")
add_compile_options("--sysroot=${CROSS_ROOTFS}")

## Specify the toolchain
set(TOOLCHAIN "i586-tizen-linux-gnu")
set(TOOLCHAIN_PREFIX ${TOOLCHAIN}-)

if ("$ENV{CC}" MATCHES "lang")
    add_compile_options(--target=i686-tizen-linux-gnu)
endif()

if (DEFINED ENV{ENV_GCC_LIB_PATH})
    set(GCC_LIB_PATH $ENV{ENV_GCC_LIB_PATH})
else()
    set(GCC_LIB_PATH /usr/lib/gcc/${TOOLCHAIN}/9.2.0)
endif()

if ("$ENV{CC}" MATCHES "lang")
    set(CROSS_LINK_FLAGS "${CROSS_LINK_FLAGS} -target i686-linux-gnu")
    set(CROSS_LINK_FLAGS "${CROSS_LINK_FLAGS} -B${CROSS_ROOTFS}${GCC_LIB_PATH}")
endif()
set(CROSS_LINK_FLAGS "${CROSS_LINK_FLAGS} -L${CROSS_ROOTFS}${GCC_LIB_PATH}")
set(CROSS_LINK_FLAGS "${CROSS_LINK_FLAGS} --sysroot=${CROSS_ROOTFS}")

set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    ${CROSS_LINK_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${CROSS_LINK_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${CROSS_LINK_FLAGS}" CACHE STRING "" FORCE)

