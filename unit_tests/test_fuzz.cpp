#include <gtest/gtest.h>
#include <random>
#include <unordered_map>
#include "oa_map.hpp"

void run_fuzz(uint32_t seed, int steps, int key_range, size_t capacity, double load) {
    mt19937 rng(seed);
    uniform_int_distribution<int> op_dist(0, 99);
    uniform_int_distribution<int> key_dist(0, key_range - 1);
    uniform_int_distribution<int> value_dist(0, 1'000'000);

    OA_Map<int, int> mine(capacity, load, -1, -2);
    unordered_map<int, int> oracle;

    for (int step = 0; step < steps; ++step) {
        const int op = op_dist(rng);
        const int key = key_dist(rng);

        if (op < 35) {
            const int value = value_dist(rng);
            mine.insert(key, value);
            oracle.insert({key, value});
        }

        else if (op < 50) {
            const int value = value_dist(rng);
            mine[key] = value;
            oracle[key] = value;
        }

        else if (op < 80) {
            mine.erase(key);
            oracle.erase(key);
        }

        else {
            auto it = mine.find(key);
            auto expected = oracle.find(key);
            ASSERT_EQ(it == mine.end(), expected == oracle.end())
                << "seed " << seed << ", step " << step << ", find " << key;
            if (expected != oracle.end()) {
                ASSERT_EQ(it->second, expected->second)
                    << "seed " << seed << ", step " << step << ", find " << key;
            }
        }

        ASSERT_GE(mine.size(), oracle.size()) << "seed " << seed << ", step " << step;
    }

    size_t count = 0;
    for (auto& [key, value] : mine) {
        auto expected = oracle.find(key);
        ASSERT_NE(expected, oracle.end()) << "seed " << seed << ", extra key " << key;
        ASSERT_EQ(value, expected->second) << "seed " << seed << ", key " << key;
        ++count;
    }

    ASSERT_EQ(count, oracle.size()) << "seed " << seed;
}

TEST(FuzzTest, SmallKeyRange) {
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        run_fuzz(seed, 5000, 64, 8, 0.875);
    }
}

TEST(FuzzTest, LargeKeyRange) {
    for (uint32_t seed = 1; seed <= 10; ++seed) {
        run_fuzz(seed, 20000, 5000, 8, 0.875);
    }
}

TEST(FuzzTest, LongChurn) {
    for (uint32_t seed = 1; seed <= 3; ++seed) {
        run_fuzz(seed, 200000, 1000, 4, 0.875);
    }
}

TEST(FuzzTest, LowLoadFactor) {
    for (uint32_t seed = 1; seed <= 5; ++seed) {
        run_fuzz(seed, 20000, 500, 8, 0.5);
    }
}
