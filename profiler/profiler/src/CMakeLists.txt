if(NOT CLR_ARCH)
  set(CLR_ARCH armel)
endif(NOT CLR_ARCH)

set(PROFILER_LIBRARIES
  ${CLR_BIN_DIR}/lib/libcoreclrpal.a)

IF(${CLR_ARCH} MATCHES "x64")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_TARGET_ARM_ -DAMD64 -DBIT64=1 -DLINUX64 -DPLATFORM_UNIX=1 -DPROFILING_SUPPORTED -DUNICODE -DUNIX_AMD64_ABI -D_AMD64_  -D_TARGET_AMD64_=1 -D_UNICODE -D_WIN64 -Wno-inline-asm -fPIC -fms-extensions -DPLATFORM_UNIX -std=c++11")

ELSE(${CLR_ARCH} MATCHES "x64")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inline-asm -fPIC -fms-extensions -DPLATFORM_UNIX")

ENDIF(${CLR_ARCH} MATCHES "x64")

set(PROFILER_SOURCES
  classfactory.cpp
  profiler.cpp
  pal_excerpts.cpp)

set(CLR_INCLUDES
  ${CLR_SRC_DIR}/src/pal/inc/rt
  ${CLR_SRC_DIR}/src/pal/prebuilt/inc
  ${CLR_SRC_DIR}/src/pal/inc 
  ${CLR_SRC_DIR}/src/inc
  ${CLR_BIN_DIR}/inc)
include_directories(AFTER ${CLR_INCLUDES})

add_library(profiler SHARED ${PROFILER_SOURCES})
target_link_libraries(profiler ${PROFILER_LIBRARIES})

install(TARGETS profiler DESTINATION lib)
