cmake_minimum_required(VERSION 3.16)
project(lfqueue-benchmarks CXX)

set(CMAKE_CXX_STANDARD 20)

set(ALLOCATOR "system" CACHE STRING "Choose global allocator override [system|mimalloc]")
option(CASCADE_LAKE "using -march=cascadelake -mtune=cascadelake" OFF)
option(STATIC "using static linking" OFF)

# allocator libraries
if(ALLOCATOR MATCHES "mimalloc")
    # mimalloc build options
    set(MI_BUILD_SHARED CACHE BOOL OFF)
    set(MI_BUILD_OBJECT CACHE BOOL OFF)
    set(MI_BUILD_TESTS  CACHE BOOL OFF)

    add_subdirectory(lib/mimalloc)
endif()

MESSAGE("using allocator: ${ALLOCATOR}")

if(STATIC)
    MESSAGE("using static linking")
endif()

# looqueue build options
add_subdirectory(lib/looqueue)
add_subdirectory(lib/scqueue)
add_subdirectory(lib/ymcq)

find_package(Threads REQUIRED)

# bench throughput
add_executable(bench_throughput
        src/bench_throughput.cpp
        src/common.cpp)
target_include_directories(bench_throughput PRIVATE include)
target_link_libraries(bench_throughput PRIVATE
        Threads::Threads
        scqueue
        looqueue
        ymcq)

# bench dequeue
add_executable(bench_dequeue
        src/bench_deque.cpp
        src/common.cpp)
target_include_directories(bench_dequeue PRIVATE include)
target_link_libraries(bench_dequeue PRIVATE
        Threads::Threads
        scqueue
        looqueue
        ymcq)

if(ALLOCATOR MATCHES "mimalloc")
    target_link_libraries(bench_throughput PRIVATE mimalloc-static)
endif()

if(STATIC)
    target_link_libraries(bench_throughput PRIVATE -static -static-libgcc -static-libstdc++)
endif()

add_executable(test_queue test/test_queue.cpp src/common.cpp)
target_include_directories(test_queue PRIVATE include/)
target_link_libraries(test_queue PRIVATE
        Threads::Threads
        looqueue
        scqueue
        ymcq)
