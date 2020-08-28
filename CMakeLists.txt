cmake_minimum_required(VERSION 3.17)
cmake_policy(SET CMP0077 NEW)

project(looqueue-benchmarks CXX)

set(ALLOCATOR "system" CACHE STRING "Choose global allocator override [system|mimalloc|rpmalloc]")
option(CASCADE_LAKE "using -march=cascadelake -mtune=cascadelake" OFF)
option(STATIC "using static linking" OFF)

# allocator libraries
if(ALLOCATOR MATCHES "mimalloc")
    # mimalloc build options
    set(MI_BUILD_SHARED OFF)
    set(MI_BUILD_OBJECT OFF)
    set(MI_BUILD_TESTS  OFF)
    set(MI_USE_CXX      OFF)

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

find_package(Threads REQUIRED)
find_package(Boost 1.68.0 REQUIRED thread)

# looqueue build options (ineffective)
add_subdirectory(lib/looqueue)
set(LOO_USE_SEQ_CST ON)

# bench throughput
add_executable(bench_throughput
        src/bench_throughput.cpp
        src/common.cpp)
target_include_directories(bench_throughput PRIVATE include)
target_compile_definitions(bench_throughput PRIVATE LOO_USE_SEQ_CST)
target_link_libraries(bench_throughput PRIVATE
        ${Boost_LIBRARIES}
        Threads::Threads
        looqueue)

if(ALLOCATOR MATCHES "mimalloc")
    target_link_libraries(bench_throughput PRIVATE mimalloc-static)
elseif(ALLOCATOR MATCHES "rpmalloc")
    target_link_libraries(bench_throughput PRIVATE rpmalloc)
endif()

if(STATIC)
    target_link_libraries(bench_throughput PRIVATE -static -static-libgcc -static-libstdc++)
endif()
