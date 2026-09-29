# Advanced-Algorithms-Assignment-1

Open-addressing hash maps in C++23, built step by step towards a Swiss table, the design behind Abseil's `flat_hash_map` and Rust's `hashbrown`. Programming Assignment 1 for Advanced Algorithms at UTS, Track A: implementation and empirical study.

## Status

In progress.

| Variant | Description | State |
|---|---|---|
| V1 | Linear probing, keys stored directly in the slots, sentinel keys for empty and deleted slots | Done, tested |
| V2 | Separate control bytes, compared one at a time | Not started |
| V3 | Control bytes scanned 16 at a time with SSE2 | Not started |

Benchmarks against `std::unordered_map` and `boost::unordered_flat_map` come after V3.

## Build and test

Requires CMake 3.24 or later, Ninja, and a C++23 compiler. GoogleTest is downloaded at configure time.

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Or run the test executable directly, which accepts GoogleTest's own options:

```
./build/swiss_tests
./build/swiss_tests --gtest_filter=OAMapTest.*
./build/swiss_tests --gtest_filter=-FuzzTest/*
```

With no build type given, the build defaults to Release. Tests keep their `assert()`s in every build type.

A second build directory runs the tests with sanitisers:

```
cmake -S . -B build-san -G Ninja -DSWISS_SANITIZE=ON
cmake --build build-san
ctest --test-dir build-san --output-on-failure
```

| CMake option | Default | Effect |
|---|---|---|
| `SWISS_BUILD_TESTS` | `ON` | Build `swiss_tests` |
| `SWISS_SANITIZE` | `OFF` | AddressSanitizer and UBSan where available. On MinGW, UBSan in trap mode only, plus libstdc++ bounds checks. |
| `SWISS_NATIVE` | `OFF` | Add `-march=native`. Off by default so results do not depend on one CPU's instruction set. |

## Layout

| Path | Contents |
|---|---|
| `src/oa_map.hpp` | V1: `OA_Map<K, V>`, linear probing open addressing, with its iterator |
| `src/hash.hpp` | `SplitMix64<K>`, the hash used by every structure |
| `unit_tests/test_maps.cpp` | Tests for each operation, rehashing, iteration and churn, run against every map over `int`, `long long` and `std::string` keys |
| `unit_tests/test_oa_map.cpp` | V1 only: sentinel keys are rejected by insert and ignored by find and erase |
| `unit_tests/test_oa_map_typed.cpp` | V1 only: inserting a sentinel throws for every key type |
| `unit_tests/test_scalar_swiss_map.cpp` | V2 only: keys that are V1 sentinels or match control-byte values behave as normal keys |
| `unit_tests/test_swiss_map.cpp` | V3 only: probe groups that wrap past the end of the table, the mirrored control bytes, iteration at the last slot, the minimum capacity of 16, repeated rehashing, and the same special keys as V2 |
| `unit_tests/test_fuzz.cpp` | Random operation sequences checked against `std::unordered_map`, run against every map |
| `unit_tests/test_hash.cpp` | Hash determinism, distribution and avalanche |
| `unit_tests/map_factory.hpp` | Builds each map type for a given key type, so shared tests are written once |
| `unit_tests/key_traits.hpp` | Sentinel values, sample keys and a test-name suffix for each tested key type |
| `ai-failures.md` | Log of AI mistakes and how they were caught, kept as they happen |

## V1 design

- **Capacity** is always a power of two. A requested capacity is rounded up with `std::bit_ceil`, so the starting slot is `hash & (capacity - 1)` rather than a division.
- **Empty and deleted slots** are marked by two sentinel keys chosen by the caller, for example `-1` and `-2` for integer keys. Inserting either sentinel throws `std::invalid_argument`. Finding or erasing one returns as if the key were absent.
- **Erase** writes a tombstone rather than an empty marker, so keys stored further along the same probe sequence stay reachable.
- **`size()` counts tombstones.** It is the number of occupied slots, not the number of live keys, and it drops back to the live count at the next rehash. This is what guarantees an empty slot always exists, so every probe loop terminates.
- **Rehash** happens before an insert that would reach the maximum load factor. It doubles the capacity and reinserts only the live keys, which clears every tombstone.
- **`insert`** does not overwrite an existing key, matching `std::unordered_map::insert`. It returns an iterator to the entry. `operator[]` inserts a default value for a missing key.

## Hash function

Every structure uses the same hash, `SplitMix64<K>`: `std::hash<K>` followed by the splitmix64 finaliser. For integer keys on libstdc++, `std::hash` is the identity function, so the finaliser does all the mixing. Each of its steps is invertible, so distinct 64-bit inputs never share a full hash. Collisions come only from masking the hash down to a slot index.

It is a type rather than a function so the same hash can be passed to the baselines, for example `std::unordered_map<K, V, SplitMix64<K>>`. That keeps hash quality from becoming an uncontrolled difference between structures.

## Environment

Recorded with every result set.

| | |
|---|---|
| Compiler | GCC 16.2.0, MSYS2 MinGW-w64 (Rev4) |
| Build tools | CMake 4.4.3, Ninja 1.13.2 |
| Flags | `-O3 -DNDEBUG -std=c++23`, no `-march` |
| CPU | AMD Ryzen 5 5600G, 6 cores / 12 threads |
| Caches | L1d 32 KB per core, L2 512 KB per core, L3 16 MB shared |
| OS | Windows 11 Pro 10.0.26200 |
| Power plan | Balanced |

**Why MinGW GCC rather than MSVC.** GCC is the same compiler family used on Linux, so flags, intrinsics and code generation carry over to a Linux run without changes. MinGW ships no AddressSanitizer runtime, so full sanitiser runs need WSL or Linux.
