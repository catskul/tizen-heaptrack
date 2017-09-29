include_directories(
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)

add_library(sharedprint STATIC
    accumulatedtracedata.cpp
)

target_link_libraries(sharedprint LINK_PUBLIC
    ${Boost_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

if (HEAPTRACK_BUILD_GUI)
    add_subdirectory(print)
endif()

if (KF5_FOUND)
    add_subdirectory(gui)
endif()
