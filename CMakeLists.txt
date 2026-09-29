cmake_minimum_required(VERSION 3.24)

project(swiss_table
    VERSION 0.1.0
    DESCRIPTION "Swiss-table hash map with benchmarks against baselines"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)            # -std=c++23, not -std=gnu++23
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)    # compile_commands.json for clangd / IntelliSense

# Default to Release for single-config generators (Ninja, MinGW Makefiles).
# Benchmarks from a Debug build are meaningless, so this guards against forgetting.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SWISS_BUILD_TESTS      "Build unit tests"                          ON)
option(SWISS_BUILD_BENCHMARKS "Build benchmarks"                          ON)
option(SWISS_SANITIZE         "Build with sanitisers (tests only)"        OFF)
option(SWISS_NATIVE           "Compile with -march=native"                OFF)

# ---------------------------------------------------------------------------
# The library: header-only, so an INTERFACE target
# ---------------------------------------------------------------------------
# An INTERFACE library compiles nothing itself. It carries include paths and
# flags to whatever links against it.

add_library(swiss_table INTERFACE)
add_library(swiss::swiss_table ALIAS swiss_table)
target_include_directories(swiss_table INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(swiss_table INTERFACE cxx_std_23)

if(SWISS_NATIVE)
    target_compile_options(swiss_table INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-march=native>)
endif()

# Warnings live in their own target so they apply to this project's code only,
# not to GoogleTest or Google Benchmark.
add_library(swiss_warnings INTERFACE)
target_compile_options(swiss_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->)

# ---------------------------------------------------------------------------
# Dependencies, downloaded at configure time
# ---------------------------------------------------------------------------

include(FetchContent)

if(SWISS_BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)   # MSVC only, harmless elsewhere
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

if(SWISS_BUILD_BENCHMARKS)
    FetchContent_Declare(benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    # Benchmark builds itself with -Werror by default. GCC 16 raises a -Wswitch
    # warning in its sysinfo.cc, which -Werror turns into a build failure.
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)

    # Benchmark's CMake does not mark its headers SYSTEM, so the project's
    # warning flags would apply to them. Copying its include directories into
    # the SYSTEM property makes every consumer get them through -isystem.
    # FetchContent_Declare(... SYSTEM) does the same but needs CMake 3.25.
    get_target_property(benchmark_includes benchmark INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(benchmark PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${benchmark_includes}")

    # Boost.Unordered is header-only, so no components are needed.
    # Boost::headers is an imported target, and CMake already treats the
    # include directories of imported targets as SYSTEM.
    find_package(Boost 1.81 REQUIRED CONFIG)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(SWISS_BUILD_TESTS)
    enable_testing()

    add_executable(swiss_tests
        unit_tests/test_fuzz.cpp
        unit_tests/test_hash.cpp
        unit_tests/test_maps.cpp
        unit_tests/test_oa_map.cpp
        unit_tests/test_oa_map_typed.cpp
        unit_tests/test_scalar_swiss_map.cpp
        unit_tests/test_swiss_map.cpp)
    target_link_libraries(swiss_tests PRIVATE
        swiss::swiss_table swiss_warnings GTest::gtest_main)
    # Tests keep their assert()s even in a Release build. -UNDEBUG comes after
    # the build type's -DNDEBUG on the command line, so it wins.
    target_compile_options(swiss_tests PRIVATE -UNDEBUG)

    if(SWISS_SANITIZE)
        if(MINGW)
            # MinGW GCC ships no ASan or UBSan runtime. UBSan in trap mode needs
            # no runtime, so it still works. It aborts instead of printing a report.
            message(STATUS "MinGW: AddressSanitizer unavailable, using UBSan trap mode only")
            set(SWISS_SAN_FLAGS -fsanitize=undefined -fsanitize-undefined-trap-on-error)
        else()
            set(SWISS_SAN_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
        endif()
        target_compile_options(swiss_tests PRIVATE ${SWISS_SAN_FLAGS})
        target_link_options(swiss_tests PRIVATE ${SWISS_SAN_FLAGS})
        # Bounds-checks std::vector::operator[] and similar in libstdc++. A
        # partial stand-in for AddressSanitizer where it is unavailable.
        target_compile_definitions(swiss_tests PRIVATE _GLIBCXX_ASSERTIONS)
    endif()

    include(GoogleTest)
    gtest_discover_tests(swiss_tests)
endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------

if(SWISS_BUILD_BENCHMARKS)
    add_executable(swiss_bench
        bench/bench_maps.cpp)
    target_link_libraries(swiss_bench PRIVATE
        swiss::swiss_table swiss_warnings Boost::headers benchmark::benchmark_main)
endif()
