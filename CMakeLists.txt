cmake_minimum_required(VERSION 3.0)
project(looqueue-benchmarks CXX)

set(CMAKE_CXX_STANDARD 20)

set(ALLOCATOR "system" CACHE STRING "Choose global allocator override [system|mimalloc|rpmalloc]")
option(CASCADE_LAKE "using -march=cascadelake -mtune=cascadelake" OFF)
option(STATIC "using static linking" OFF)

# allocator libraries
if(ALLOCATOR MATCHES "mimalloc")
    # mimalloc build options
    set(MI_BUILD_SHARED CACHE BOOL OFF)
    set(MI_BUILD_OBJECT CACHE BOOL OFF)
    set(MI_BUILD_TESTS  CACHE BOOL OFF)
    set(MI_USE_CXX      CACHE BOOL OFF)

    add_subdirectory(lib/mimalloc)
elseif(ALLOCATOR MATCHES "rpmalloc")
    add_subdirectory(lib)
endif()

MESSAGE("using allocator: ${ALLOCATOR}")

if (CASCADE_LAKE)
    MESSAGE("using '-march=cascadelake -mtune=cascadelake' (Cascade Lake CPU)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=cascadelake -mtune=cascadelake")
endif()

if(STATIC)
    MESSAGE("using static linking")
    set(Boost_USE_STATIC_LIBS ON)
endif()

# looqueue build options
set(LOO_USE_SEQ_CST CACHE BOOL ON)
add_subdirectory(lib/looqueue)
add_subdirectory(lib/scqueue)
add_subdirectory(lib/ymcqueue)

find_package(Threads REQUIRED)
find_package(Boost 1.68.0 REQUIRED thread)


# bench throughput
add_executable(bench_throughput
        src/bench_throughput.cpp
        src/common.cpp)
target_include_directories(bench_throughput PRIVATE include)
target_link_libraries(bench_throughput PRIVATE
        # we use a local static version of libatomic
        ${CMAKE_SOURCE_DIR}/lib/libatomic.a
        ${Boost_LIBRARIES}
        Threads::Threads
        scqueue
        looqueue
        ymcqueue)

if(ALLOCATOR MATCHES "mimalloc")
    target_link_libraries(bench_throughput PRIVATE mimalloc-static)
elseif(ALLOCATOR MATCHES "rpmalloc")
    target_link_libraries(bench_throughput PRIVATE rpmalloc)
endif()

if(STATIC)
    target_link_libraries(bench_throughput PRIVATE -static -static-libgcc -static-libstdc++)
endif()
