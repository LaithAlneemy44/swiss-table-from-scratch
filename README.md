# Advanced-Algorithms-Assignment-1

## Requirements

- CMake 3.24 or later, Ninja, and a C++23 compiler
- Boost 1.81 or later, for the benchmark baseline (MSYS2 package `mingw-w64-x86_64-boost`)
- Python 3 with matplotlib, for the plots

GoogleTest and Google Benchmark are downloaded at configure time.

## Build

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Unit tests

```
ctest --test-dir build --output-on-failure
```

Or run the test executable directly, which accepts GoogleTest's own options:

```
./build/swiss_tests
./build/swiss_tests --gtest_filter=OAMapTest.*
```

Tests keep their `assert()`s in every build type. To run them with sanitisers, use a second build directory:

```
cmake -S . -B build-san -G Ninja -DSWISS_SANITIZE=ON
cmake --build build-san
ctest --test-dir build-san --output-on-failure
```

## Benchmarks

`swiss_bench` is built from `bench/bench_maps.cpp`. A full run takes about 25 minutes and writes the results as JSON:

```
mkdir results
./build/swiss_bench --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_enable_random_interleaving=true --benchmark_out=results/bench.json --benchmark_out_format=json
```

To run a subset, add `--benchmark_filter`, for example `--benchmark_filter=^E2/`.

## Plots

`tools/plot.py` reads the benchmark JSON and writes one PNG per plot, plus `summary.csv` with every plotted value:

```
pip install matplotlib
python tools/plot.py results/bench.json results/plots
```
