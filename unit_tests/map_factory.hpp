#pragma once
#include <cstddef>
#include <string>
#include "oa_map.hpp"
#include "scalar_swiss_map.hpp"
#include "key_traits.hpp"

template<typename K, typename V = int>
struct OAMapFactory {
    using Key = K;
    using Map = OA_Map<K, V>;

    static Map make(size_t capacity, double max_load_factor) {
        return Map(capacity, max_load_factor, KeyTraits<K>::empty(), KeyTraits<K>::deleted());
    }

    static string name() {
        return "OAMap" + KeyTraits<K>::name();
    }
};

template<typename K, typename V = int>
struct ScalarSwissMapFactory {
    using Key = K;
    using Map = Scalar_Swiss_Map<K, V>;

    static Map make(size_t capacity, double max_load_factor) {
        return Map(capacity, max_load_factor);
    }

    static string name() {
        return "ScalarSwissMap" + KeyTraits<K>::name();
    }
};

struct MapTypeNames {
    template<typename Factory>
    static string GetName(int) {
        return Factory::name();
    }
};
