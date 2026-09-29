#include <gtest/gtest.h>
#include <array>
#include <set>
#include "scalar_swiss_map.hpp"

constexpr array<int, 5> special_keys{-1, -2, 0, 128, 255};

TEST(ScalarSwissMapTest, InsertSpecialKeys) {
    Scalar_Swiss_Map<int, int> map(32, 0.875);
    for (int key : special_keys) {
        auto it = map.insert(key, key * 10);
        ASSERT_NE(it, map.end()) << "key " << key;
        EXPECT_EQ(it->first, key);
        EXPECT_EQ(it->second, key * 10);
    }

    EXPECT_EQ(map.size(), special_keys.size());
}

TEST(ScalarSwissMapTest, SubscriptSpecialKeys) {
    Scalar_Swiss_Map<int, int> map(32, 0.875);
    for (int key : special_keys) {
        EXPECT_EQ(map[key], 0) << "key " << key;
        map[key] = key * 10;
    }

    EXPECT_EQ(map.size(), special_keys.size());
    for (int key : special_keys) {
        EXPECT_EQ(map[key], key * 10) << "key " << key;
    }
}

TEST(ScalarSwissMapTest, FindSpecialKeys) {
    Scalar_Swiss_Map<int, int> map(32, 0.875);
    for (int key : special_keys) {
        EXPECT_EQ(map.find(key), map.end()) << "key " << key;
    }

    for (int key : special_keys) {
        map.insert(key, key * 10);
    }

    for (int key : special_keys) {
        auto it = map.find(key);
        ASSERT_NE(it, map.end()) << "key " << key;
        EXPECT_EQ(it->first, key);
        EXPECT_EQ(it->second, key * 10);
    }
}

TEST(ScalarSwissMapTest, EraseSpecialKeys) {
    Scalar_Swiss_Map<int, int> map(32, 0.875);
    for (int key : special_keys) {
        map.insert(key, key * 10);
    }

    for (size_t i = 0; i < special_keys.size(); ++i) {
        map.erase(special_keys[i]);
        for (size_t j = 0; j < special_keys.size(); ++j) {
            EXPECT_EQ(map.find(special_keys[j]) == map.end(), j <= i)
                << "erased " << special_keys[i] << ", key " << special_keys[j];
        }
    }

    EXPECT_EQ(map.size(), special_keys.size());
    EXPECT_EQ(map.begin(), map.end());
}

TEST(ScalarSwissMapTest, IterateSpecialKeys) {
    Scalar_Swiss_Map<int, int> map(32, 0.875);
    for (int key : special_keys) {
        map.insert(key, key * 10);
    }

    set<int> seen;
    for (auto& [key, value] : map) {
        EXPECT_TRUE(seen.insert(key).second) << "key " << key;
        EXPECT_EQ(value, key * 10);
    }

    EXPECT_EQ(seen, set<int>(special_keys.begin(), special_keys.end()));
}

TEST(ScalarSwissMapTest, SpecialKeysSurviveRehash) {
    Scalar_Swiss_Map<int, int> map(4, 0.875);
    for (int key : special_keys) {
        map.insert(key, key * 10);
    }

    for (int i = 1000; i < 2000; ++i) {
        map.insert(i, i);
    }

    for (int key : special_keys) {
        auto it = map.find(key);
        ASSERT_NE(it, map.end()) << "key " << key;
        EXPECT_EQ(it->second, key * 10);
    }
}
