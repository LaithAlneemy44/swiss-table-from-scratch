#include <benchmark/benchmark.h>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/version.hpp>
#include <algorithm>
#include <bit>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <malloc.h>
#include <new>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "hash.hpp"
#include "oa_map.hpp"
#include "scalar_swiss_map.hpp"
#include "swiss_map.hpp"

template<typename K>
struct BenchHash : SplitMix64<K> {
    using is_avalanching = std::true_type;
};

namespace {

// Live bytes requested through operator new. Unaligned blocks carry no header,
// so std and Boost node sizes in the timed benchmarks are unchanged. An unsized
// delete recovers the size from the heap: on the Windows CRT heap _msize returns
// the requested size exactly. Elsewhere malloc_usable_size can exceed it, which
// only affects frees, and none happen while a map is being measured.
size_t g_live_bytes = 0;

struct AlignedHeader {
    void* raw;
    size_t bytes;
};

void* checked_malloc(size_t bytes) {
    for (;;) {
        if (void* p = malloc(bytes)) {
            return p;
        }

        const new_handler handler = get_new_handler();
        if (!handler) {
            throw bad_alloc();
        }

        handler();
    }
}

size_t heap_block_size(void* p) {
#ifdef _WIN32
    return _msize(p);
#else
    return malloc_usable_size(p);
#endif
}

void* tracked_new(size_t size) {
    const size_t bytes = max(size, size_t{1});
    void* p = checked_malloc(bytes);
    g_live_bytes += bytes;
    return p;
}

void tracked_delete(void* p, size_t bytes) noexcept {
    if (p) {
        g_live_bytes -= bytes;
        free(p);
    }
}

void tracked_delete(void* p) noexcept {
    if (p) {
        tracked_delete(p, heap_block_size(p));
    }
}

void* tracked_aligned_new(size_t size, align_val_t alignment) {
    const size_t align = static_cast<size_t>(alignment);
    const size_t bytes = max(size, size_t{1});
    void* raw = checked_malloc(bytes + align + sizeof(AlignedHeader));
    const uintptr_t first = reinterpret_cast<uintptr_t>(raw) + sizeof(AlignedHeader);
    const uintptr_t aligned = (first + align - 1) & ~(align - 1);
    AlignedHeader* header = reinterpret_cast<AlignedHeader*>(aligned) - 1;
    header->raw = raw;
    header->bytes = bytes;
    g_live_bytes += bytes;
    return reinterpret_cast<void*>(aligned);
}

void tracked_aligned_delete(void* p) noexcept {
    if (p) {
        const AlignedHeader* header = static_cast<const AlignedHeader*>(p) - 1;
        g_live_bytes -= header->bytes;
        free(header->raw);
    }
}

template<typename Allocate>
void* nothrow_allocate(Allocate allocate) noexcept {
    try {
        return allocate();
    }

    catch (...) {
        return nullptr;
    }
}

}

void* operator new(size_t size) {
    return tracked_new(size);
}

void* operator new[](size_t size) {
    return tracked_new(size);
}

void* operator new(size_t size, const nothrow_t&) noexcept {
    return nothrow_allocate([size] { return tracked_new(size); });
}

void* operator new[](size_t size, const nothrow_t&) noexcept {
    return nothrow_allocate([size] { return tracked_new(size); });
}

void* operator new(size_t size, align_val_t alignment) {
    return tracked_aligned_new(size, alignment);
}

void* operator new[](size_t size, align_val_t alignment) {
    return tracked_aligned_new(size, alignment);
}

void* operator new(size_t size, align_val_t alignment, const nothrow_t&) noexcept {
    return nothrow_allocate([size, alignment] { return tracked_aligned_new(size, alignment); });
}

void* operator new[](size_t size, align_val_t alignment, const nothrow_t&) noexcept {
    return nothrow_allocate([size, alignment] { return tracked_aligned_new(size, alignment); });
}

void operator delete(void* p) noexcept {
    tracked_delete(p);
}

void operator delete[](void* p) noexcept {
    tracked_delete(p);
}

void operator delete(void* p, size_t size) noexcept {
    tracked_delete(p, max(size, size_t{1}));
}

void operator delete[](void* p, size_t size) noexcept {
    tracked_delete(p, max(size, size_t{1}));
}

void operator delete(void* p, const nothrow_t&) noexcept {
    tracked_delete(p);
}

void operator delete[](void* p, const nothrow_t&) noexcept {
    tracked_delete(p);
}

void operator delete(void* p, align_val_t) noexcept {
    tracked_aligned_delete(p);
}

void operator delete[](void* p, align_val_t) noexcept {
    tracked_aligned_delete(p);
}

void operator delete(void* p, size_t, align_val_t) noexcept {
    tracked_aligned_delete(p);
}

void operator delete[](void* p, size_t, align_val_t) noexcept {
    tracked_aligned_delete(p);
}

void operator delete(void* p, align_val_t, const nothrow_t&) noexcept {
    tracked_aligned_delete(p);
}

void operator delete[](void* p, align_val_t, const nothrow_t&) noexcept {
    tracked_aligned_delete(p);
}

namespace {

constexpr uint32_t present_seed = 1;
constexpr uint32_t miss_seed = 2;
constexpr uint32_t shuffle_seed = 3;
constexpr uint32_t churn_seed = 4;

constexpr double target_load = 0.75;
constexpr double custom_max_load = 0.95;

template<typename K>
struct Keys;

template<>
struct Keys<int> {
    static constexpr const char* name = "int";
    static constexpr size_t pool_size = size_t{1} << 22;

    static int empty() {
        return INT_MIN;
    }

    static int deleted() {
        return INT_MIN + 1;
    }

    static int draw(mt19937& rng) {
        return uniform_int_distribution<int>(INT_MIN + 2, INT_MAX)(rng);
    }
};

template<>
struct Keys<string> {
    static constexpr const char* name = "string";
    static constexpr size_t pool_size = size_t{1} << 20;

    static string empty() {
        return "#empty";
    }

    static string deleted() {
        return "#deleted";
    }

    static string draw(mt19937& rng) {
        const uint64_t high = rng();
        const uint64_t low = rng();
        return format("{:016x}", (high << 32) | low);
    }
};

template<typename K>
vector<K> distinct_keys(uint32_t seed, size_t count, const unordered_set<K>& excluded) {
    mt19937 rng(seed);
    unordered_set<K> seen;
    seen.reserve(count);
    vector<K> keys;
    keys.reserve(count);
    while (keys.size() < count) {
        K key = Keys<K>::draw(rng);
        if (!excluded.contains(key) && seen.insert(key).second) {
            keys.push_back(move(key));
        }
    }

    return keys;
}

template<typename K>
const vector<K>& present_pool() {
    static const vector<K> pool = distinct_keys<K>(present_seed, Keys<K>::pool_size, {});
    return pool;
}

template<typename K>
const vector<K>& miss_pool() {
    static const vector<K> pool = [] {
        const vector<K>& present = present_pool<K>();
        const unordered_set<K> excluded(present.begin(), present.end());
        return distinct_keys<K>(miss_seed, Keys<K>::pool_size, excluded);
    }();
    return pool;
}

template<typename K>
vector<K> shuffled_prefix(const vector<K>& pool, size_t count) {
    vector<size_t> order(count);
    iota(order.begin(), order.end(), size_t{0});
    shuffle(order.begin(), order.end(), mt19937(shuffle_seed));
    vector<K> keys;
    keys.reserve(count);
    for (size_t i : order) {
        keys.push_back(pool[i]);
    }

    return keys;
}

size_t presized_capacity(size_t n, double load) {
    return bit_ceil(static_cast<size_t>(ceil(static_cast<double>(n) / load)));
}

template<typename K>
struct V1 {
    using Map = OA_Map<K, int>;
    static constexpr const char* name = "V1";

    static double max_load_factor() {
        return custom_max_load;
    }

    static Map with_capacity(size_t capacity) {
        return Map(capacity, custom_max_load, Keys<K>::empty(), Keys<K>::deleted());
    }

    static Map presized(size_t n) {
        return with_capacity(presized_capacity(n, target_load));
    }
};

template<typename K>
struct V2 {
    using Map = Scalar_Swiss_Map<K, int>;
    static constexpr const char* name = "V2";

    static double max_load_factor() {
        return custom_max_load;
    }

    static Map with_capacity(size_t capacity) {
        return Map(capacity, custom_max_load);
    }

    static Map presized(size_t n) {
        return with_capacity(presized_capacity(n, target_load));
    }
};

template<typename K>
struct V3 {
    using Map = Swiss_Map<K, int>;
    static constexpr const char* name = "V3";

    static double max_load_factor() {
        return custom_max_load;
    }

    static Map with_capacity(size_t capacity) {
        return Map(capacity, custom_max_load);
    }

    static Map presized(size_t n) {
        return with_capacity(presized_capacity(n, target_load));
    }
};

template<typename K>
struct Std {
    using Map = unordered_map<K, int, BenchHash<K>>;
    static constexpr const char* name = "Std";

    static Map presized(size_t n) {
        Map map;
        map.reserve(n);
        return map;
    }
};

template<typename K>
struct Boost {
    using Map = boost::unordered_flat_map<K, int, BenchHash<K>>;
    static constexpr const char* name = "Boost";

    static double max_load_factor() {
        return Map().max_load_factor();
    }

    // Boost capacities are 15 * 2^k - 1, so none equals a power of two. Take the
    // one nearest the requested slot count so the table's footprint matches the
    // custom maps as closely as possible.
    static Map with_capacity(size_t slots) {
        Map above;
        above.rehash(slots);
        Map below;
        below.rehash(slots / 2);
        const size_t gap_above = above.bucket_count() - slots;
        const size_t gap_below = slots - min(below.bucket_count(), slots);
        return gap_below < gap_above ? move(below) : move(above);
    }

    static Map presized(size_t n) {
        Map map;
        map.reserve(n);
        return map;
    }
};

template<typename Map, typename K>
auto insert_entry(Map& map, const K& key, int value) {
    if constexpr (requires { map.try_emplace(key, value); }) {
        return map.try_emplace(key, value);
    }

    else {
        return map.insert(key, value);
    }
}

template<typename Map, typename K>
void erase_entry(Map& map, const K& key) {
    if constexpr (is_void_v<decltype(map.erase(key))>) {
        map.erase(key);
    }

    else {
        auto erased = map.erase(key);
        benchmark::DoNotOptimize(erased);
    }
}

template<typename Map>
size_t table_capacity(Map& map) {
    if constexpr (requires { map.bucket_count(); }) {
        return map.bucket_count();
    }

    else {
        return map.capacity();
    }
}

template<typename Map>
double load_of(Map& map, size_t live) {
    if constexpr (requires { map.load_factor(); }) {
        return map.load_factor();
    }

    else {
        return static_cast<double>(live) / static_cast<double>(map.capacity());
    }
}

template<typename Map>
size_t fill_count(Map& map, double load) {
    const size_t n = static_cast<size_t>(load * static_cast<double>(table_capacity(map)));
    if constexpr (requires { map.max_load(); }) {
        return min(n, map.max_load());
    }

    else {
        return n;
    }
}

template<typename Map, typename K>
bool fill_without_rehash(Map& map, const vector<K>& keys, size_t n) {
    const size_t slots = table_capacity(map);
    for (size_t i = 0; i < n; ++i) {
        insert_entry(map, keys[i], static_cast<int>(i));
    }

    return table_capacity(map) == slots;
}

template<typename Map>
size_t live_count(Map& map) {
    size_t count = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        ++count;
    }

    return count;
}

enum class Lookup {
    Hit,
    Miss
};

template<typename K, typename Map>
void time_lookups(benchmark::State& state, Map& map, size_t n, Lookup kind) {
    const vector<K> lookups = shuffled_prefix(kind == Lookup::Hit ? present_pool<K>() : miss_pool<K>(), n);
    for (const K& key : lookups) {
        if ((map.find(key) != map.end()) != (kind == Lookup::Hit)) {
            state.SkipWithError(kind == Lookup::Hit ? "a present key was not found" : "a missing key was found");
            return;
        }
    }

    for (auto _ : state) {
        for (const K& key : lookups) {
            auto it = map.find(key);
            benchmark::DoNotOptimize(it);
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
    state.counters["load"] = load_of(map, n);
}

template<template<typename> class P, typename K>
void bm_lookup(benchmark::State& state, size_t n, Lookup kind) {
    auto map = P<K>::presized(n);
    if (!fill_without_rehash(map, present_pool<K>(), n)) {
        state.SkipWithError("table rehashed during setup");
        return;
    }

    time_lookups<K>(state, map, n, kind);
}

template<template<typename> class P, typename K>
void bm_lookup_at_load(benchmark::State& state, size_t slots, double load, Lookup kind) {
    auto map = P<K>::with_capacity(slots);
    const size_t n = fill_count(map, load);
    if (!fill_without_rehash(map, present_pool<K>(), n)) {
        state.SkipWithError("table rehashed during setup");
        return;
    }

    time_lookups<K>(state, map, n, kind);
}

// A filled map cannot be refilled, so every iteration builds a fresh presized
// map and times only the fill with steady_clock. PauseTiming/ResumeTiming would
// need two pairs per iteration, one around construction and one around the
// previous map's destruction. Each ResumeTiming starts the wall clock and then
// reads the thread CPU timer through a system call, so that call lands inside
// the measured time. Two clock reads around the fill exclude construction and
// destruction exactly and cost far less.
template<template<typename> class P, typename K>
void bm_insert(benchmark::State& state, size_t n) {
    const vector<K>& keys = present_pool<K>();
    for (auto _ : state) {
        auto map = P<K>::presized(n);
        const size_t slots = table_capacity(map);
        const auto start = chrono::steady_clock::now();
        for (size_t i = 0; i < n; ++i) {
            auto result = insert_entry(map, keys[i], static_cast<int>(i));
            benchmark::DoNotOptimize(result);
        }

        const auto stop = chrono::steady_clock::now();
        state.SetIterationTime(chrono::duration<double>(stop - start).count());
        if (table_capacity(map) != slots) {
            state.SkipWithError("table rehashed during the timed fill");
            break;
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

template<template<typename> class P, typename K>
void bm_churn(benchmark::State& state, size_t n, size_t rounds) {
    const vector<K>& keys = present_pool<K>();
    if (n + rounds > keys.size() || state.max_iterations != static_cast<benchmark::IterationCount>(rounds)) {
        state.SkipWithError("churn needs Iterations(rounds) and enough distinct keys");
        return;
    }

    mt19937 rng(churn_seed);
    uniform_int_distribution<size_t> pick(0, n - 1);
    vector<size_t> victims(rounds);
    for (size_t& victim : victims) {
        victim = pick(rng);
    }

    vector<K> live(keys.begin(), keys.begin() + static_cast<ptrdiff_t>(n));
    auto map = P<K>::presized(n);
    if (!fill_without_rehash(map, keys, n)) {
        state.SkipWithError("table rehashed during setup");
        return;
    }

    size_t done = 0;
    for (auto _ : state) {
        K& victim = live[victims[done]];
        erase_entry(map, victim);
        auto result = insert_entry(map, keys[n + done], static_cast<int>(done));
        benchmark::DoNotOptimize(result);
        victim = keys[n + done];
        ++done;
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["capacity"] = static_cast<double>(table_capacity(map));
    if (live_count(map) != n) {
        state.SkipWithError("the live count changed during churn");
    }
}

template<template<typename> class P, typename K>
void bm_iterate(benchmark::State& state, size_t n) {
    auto map = P<K>::presized(n);
    if (!fill_without_rehash(map, present_pool<K>(), n) || live_count(map) != n) {
        state.SkipWithError("setup did not produce n live entries without a rehash");
        return;
    }

    for (auto _ : state) {
        int64_t sum = 0;
        for (auto& [key, value] : map) {
            sum += value;
        }

        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
    state.counters["load"] = load_of(map, n);
}

template<template<typename> class P, typename K>
void bm_memory(benchmark::State& state, size_t n) {
    const vector<K>& keys = present_pool<K>();
    const size_t before = g_live_bytes;
    auto map = P<K>::presized(n);
    const bool rehashed = !fill_without_rehash(map, keys, n);
    const size_t bytes = g_live_bytes - before;
    if (rehashed) {
        state.SkipWithError("table rehashed during setup");
        return;
    }

    for (auto _ : state) {
        size_t size = map.size();
        benchmark::DoNotOptimize(size);
    }

    state.counters["bytes_per_element"] = static_cast<double>(bytes) / static_cast<double>(n);
    state.counters["load"] = load_of(map, n);
}

vector<size_t> sizes_up_to(size_t largest) {
    vector<size_t> sizes;
    for (size_t n = size_t{1} << 10; n <= largest; n *= 4) {
        sizes.push_back(n);
    }

    return sizes;
}

string bench_name(const char* experiment, const char* operation, const char* map, const char* key, const string& parameter) {
    return format("{}/{}/{}/{}/{}", experiment, operation, map, key, parameter);
}

// Every benchmark reports wall-clock time. Google Benchmark's CPU time on
// Windows comes from GetThreadTimes, which only advances in 15.6 ms steps.
template<template<typename> class P, typename K>
void register_e1_for(size_t largest) {
    for (size_t n : sizes_up_to(largest)) {
        const string parameter = to_string(n);
        benchmark::RegisterBenchmark(bench_name("E1", "LookupHit", P<K>::name, Keys<K>::name, parameter), bm_lookup<P, K>, n, Lookup::Hit)->UseRealTime();
        benchmark::RegisterBenchmark(bench_name("E1", "LookupMiss", P<K>::name, Keys<K>::name, parameter), bm_lookup<P, K>, n, Lookup::Miss)->UseRealTime();
        benchmark::RegisterBenchmark(bench_name("E1", "Insert", P<K>::name, Keys<K>::name, parameter), bm_insert<P, K>, n)->UseManualTime();
    }
}

template<template<typename> class P>
void register_e2_for() {
    const size_t slots = size_t{1} << 20;
    for (double load : {0.5, 0.625, 0.75, 0.875, 0.9375}) {
        if (load > P<int>::max_load_factor()) {
            continue;
        }

        const string parameter = format("{}", load);
        benchmark::RegisterBenchmark(bench_name("E2", "LookupHit", P<int>::name, "int", parameter), bm_lookup_at_load<P, int>, slots, load, Lookup::Hit)->UseRealTime();
        benchmark::RegisterBenchmark(bench_name("E2", "LookupMiss", P<int>::name, "int", parameter), bm_lookup_at_load<P, int>, slots, load, Lookup::Miss)->UseRealTime();
    }
}

template<template<typename> class P>
void register_e3_for() {
    const size_t n = size_t{1} << 16;
    for (size_t multiple : {1u, 4u, 16u}) {
        const size_t rounds = multiple * n;
        benchmark::RegisterBenchmark(bench_name("E3", "Churn", P<int>::name, "int", to_string(rounds)), bm_churn<P, int>, n, rounds)
            ->Iterations(static_cast<benchmark::IterationCount>(rounds))
            ->UseRealTime();
    }
}

template<template<typename> class P>
void register_e4_for() {
    for (size_t n : sizes_up_to(size_t{1} << 20)) {
        benchmark::RegisterBenchmark(bench_name("E4", "Iterate", P<int>::name, "int", to_string(n)), bm_iterate<P, int>, n)->UseRealTime();
    }
}

template<template<typename> class P, typename K>
void register_e5_for() {
    for (size_t n : sizes_up_to(size_t{1} << 20)) {
        benchmark::RegisterBenchmark(bench_name("E5", "Memory", P<K>::name, Keys<K>::name, to_string(n)), bm_memory<P, K>, n)
            ->Iterations(1)
            ->UseRealTime();
    }
}

template<template<typename> class... Maps>
void register_e1() {
    (register_e1_for<Maps, int>(size_t{1} << 22), ...);
    (register_e1_for<Maps, string>(size_t{1} << 20), ...);
}

template<template<typename> class... Maps>
void register_e2() {
    (register_e2_for<Maps>(), ...);
}

template<template<typename> class... Maps>
void register_e3() {
    (register_e3_for<Maps>(), ...);
}

template<template<typename> class... Maps>
void register_e4() {
    (register_e4_for<Maps>(), ...);
}

template<template<typename> class... Maps>
void register_e5() {
    (register_e5_for<Maps, int>(), ...);
    (register_e5_for<Maps, string>(), ...);
}

void add_context() {
#ifdef __VERSION__
    benchmark::AddCustomContext("compiler", __VERSION__);
#endif
    benchmark::AddCustomContext("boost_version", BOOST_LIB_VERSION);
    benchmark::AddCustomContext("custom_max_load_factor", format("{}", custom_max_load));
    benchmark::AddCustomContext("pair_string_int_bytes", to_string(sizeof(pair<string, int>)));
}

[[maybe_unused]] const bool registered = [] {
    add_context();
    register_e1<V1, V2, V3, Std, Boost>();
    register_e2<V1, V2, V3, Boost>();
    register_e3<V1, V2, V3, Std, Boost>();
    register_e4<V1, V2, V3, Std, Boost>();
    register_e5<V1, V2, V3, Std, Boost>();
    return true;
}();

}
