#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

using namespace std;

template<typename K>
struct SplitMix64 {
    size_t operator()(const K& key) const {
        uint64_t x = hash<K>{}(key);
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return static_cast<size_t>(x ^ (x >> 31));
    }
};