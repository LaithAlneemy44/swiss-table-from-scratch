#include <gtest/gtest.h>
#include <array>
#include <set>
#include <string>
#include <vector>
#include "hash.hpp"
#include "map_factory.hpp"

template<typename K>
vector<K> keys_with_home_in(size_t capacity, size_t first, size_t last, size_t count) {
    const size_t mask = capacity - 1;
    SplitMix64<K> hasher;
    vector<K> keys;
    for (int i = 0; keys.size() < count; ++i) {
        const K key = KeyTraits<K>::key(i);
        const size_t home = hasher(key) & mask;
        if (home >= first && home <= last) {
            keys.push_back(key);
        }
    }

    return keys;
}

template<typename Factory>
class SwissMapBoundaryTest : public ::testing::Test {
protected:
    static typename Factory::Key key(int i) {
        return KeyTraits<typename Factory::Key>::key(i);
    }
};

using SwissMapTypes = ::testing::Types<SwissMapFactory<int>, SwissMapFactory<long long>, SwissMapFactory<string>>;
TYPED_TEST_SUITE(SwissMapBoundaryTest, SwissMapTypes, MapTypeNames);

TYPED_TEST(SwissMapBoundaryTest, CollidingKeysAtEnd) {
    using Key = typename TypeParam::Key;
    for (size_t capacity : {16u, 32u, 64u}) {
        SCOPED_TRACE("capacity " + to_string(capacity));
        const vector<Key> keys = keys_with_home_in<Key>(capacity, capacity - 3, capacity - 1, 8);
        auto map = TypeParam::make(capacity, 0.875);
        for (size_t i = 0; i < keys.size(); ++i) {
            map.insert(keys[i], static_cast<int>(i));
        }

        for (size_t i = 0; i < keys.size(); ++i) {
            auto it = map.find(keys[i]);
            ASSERT_NE(it, map.end()) << "key " << keys[i];
            EXPECT_EQ(it->second, static_cast<int>(i));
        }

        for (size_t i = 0; i < keys.size(); i += 2) {
            map.erase(keys[i]);
        }

        set<Key> expected;
        for (size_t i = 0; i < keys.size(); ++i) {
            auto it = map.find(keys[i]);
            if (i % 2 == 0) {
                EXPECT_EQ(it, map.end()) << "key " << keys[i];
            }

            else {
                ASSERT_NE(it, map.end()) << "key " << keys[i];
                EXPECT_EQ(it->second, static_cast<int>(i));
                expected.insert(keys[i]);
            }
        }

        set<Key> seen;
        for (auto& [key, value] : map) {
            EXPECT_TRUE(seen.insert(key).second) << "key " << key;
        }

        EXPECT_EQ(seen, expected);
        EXPECT_EQ(map.size(), keys.size());
    }
}

TYPED_TEST(SwissMapBoundaryTest, MirroredControlBytes) {
    using Key = typename TypeParam::Key;
    for (size_t capacity : {32u, 64u}) {
        SCOPED_TRACE("capacity " + to_string(capacity));
        const vector<Key> front = keys_with_home_in<Key>(capacity, 0, 0, 15);
        const vector<Key> back = keys_with_home_in<Key>(capacity, capacity - 1, capacity - 1, 4);
        auto map = TypeParam::make(capacity, 0.875);
        map.insert(back[0], 100);
        for (size_t i = 0; i < front.size(); ++i) {
            map.insert(front[i], static_cast<int>(i));
        }

        map.insert(back[1], 101);
        for (size_t i = 0; i < front.size(); ++i) {
            auto it = map.find(front[i]);
            ASSERT_NE(it, map.end()) << "front key " << front[i];
            EXPECT_EQ(it->second, static_cast<int>(i));
        }

        for (size_t i = 0; i < 2; ++i) {
            auto it = map.find(back[i]);
            ASSERT_NE(it, map.end()) << "back key " << back[i];
            EXPECT_EQ(it->second, static_cast<int>(100 + i));
        }

        EXPECT_EQ(map.find(back[3]), map.end());
        for (size_t i = 0; i < front.size(); i += 2) {
            map.erase(front[i]);
        }

        auto it = map.find(back[1]);
        ASSERT_NE(it, map.end());
        EXPECT_EQ(it->second, 101);
        EXPECT_EQ(map.find(back[3]), map.end());
        map.insert(back[2], 102);
        it = map.find(back[2]);
        ASSERT_NE(it, map.end());
        EXPECT_EQ(it->second, 102);
        set<Key> expected{back[0], back[1], back[2]};
        for (size_t i = 0; i < front.size(); ++i) {
            EXPECT_EQ(map.find(front[i]) == map.end(), i % 2 == 0) << "front key " << front[i];
            if (i % 2 == 1) {
                expected.insert(front[i]);
            }
        }

        set<Key> seen;
        for (auto& [key, value] : map) {
            EXPECT_TRUE(seen.insert(key).second) << "key " << key;
        }

        EXPECT_EQ(seen, expected);
        EXPECT_EQ(map.size(), 18u);
    }
}

TYPED_TEST(SwissMapBoundaryTest, IterateLastSlotFull) {
    using Key = typename TypeParam::Key;
    for (size_t capacity : {16u, 32u, 64u}) {
        SCOPED_TRACE("capacity " + to_string(capacity));
        const vector<Key> back = keys_with_home_in<Key>(capacity, capacity - 1, capacity - 1, 4);
        const vector<Key> middle = keys_with_home_in<Key>(capacity, 4, capacity - 5, 3);
        auto map = TypeParam::make(capacity, 0.875);
        set<Key> expected;
        for (const Key& key : back) {
            map.insert(key, 1);
            expected.insert(key);
        }

        for (const Key& key : middle) {
            map.insert(key, 2);
            expected.insert(key);
        }

        set<Key> seen;
        auto last = map.end();
        for (auto it = map.begin(); it != map.end(); ++it) {
            EXPECT_TRUE(seen.insert(it->first).second) << "key " << it->first;
            last = it;
        }

        EXPECT_EQ(seen, expected);
        ASSERT_NE(last, map.end());
        EXPECT_EQ(last->first, back[0]);
        auto it = map.find(back[0]);
        ASSERT_NE(it, map.end());
        ++it;
        EXPECT_EQ(it, map.end());
    }
}

TYPED_TEST(SwissMapBoundaryTest, IterateOnlyLastSlotLive) {
    using Key = typename TypeParam::Key;
    for (size_t capacity : {16u, 32u, 64u}) {
        SCOPED_TRACE("capacity " + to_string(capacity));
        const vector<Key> back = keys_with_home_in<Key>(capacity, capacity - 1, capacity - 1, 1);
        auto map = TypeParam::make(capacity, 0.875);
        map.insert(back[0], 7);
        auto it = map.begin();
        ASSERT_NE(it, map.end());
        EXPECT_EQ(it->first, back[0]);
        EXPECT_EQ(it->second, 7);
        ++it;
        EXPECT_EQ(it, map.end());
    }
}

TYPED_TEST(SwissMapBoundaryTest, IterateOnlyLastSlotLiveAfterErase) {
    using Key = typename TypeParam::Key;
    for (size_t capacity : {16u, 32u, 64u}) {
        SCOPED_TRACE("capacity " + to_string(capacity));
        const vector<Key> back = keys_with_home_in<Key>(capacity, capacity - 1, capacity - 1, 4);
        auto map = TypeParam::make(capacity, 0.875);
        for (size_t i = 0; i < back.size(); ++i) {
            map.insert(back[i], static_cast<int>(i));
        }

        for (size_t i = 1; i < back.size(); ++i) {
            map.erase(back[i]);
        }

        auto it = map.begin();
        ASSERT_NE(it, map.end());
        EXPECT_EQ(it->first, back[0]);
        EXPECT_EQ(it->second, 0);
        ++it;
        EXPECT_EQ(it, map.end());
    }
}

TYPED_TEST(SwissMapBoundaryTest, IterateStopsBeforeMirroredBytes) {
    using Key = typename TypeParam::Key;
    for (size_t capacity : {16u, 32u, 64u}) {
        SCOPED_TRACE("capacity " + to_string(capacity));
        const vector<Key> back = keys_with_home_in<Key>(capacity, capacity - 1, capacity - 1, 3);
        const vector<Key> near_end = keys_with_home_in<Key>(capacity, capacity - 3, capacity - 3, 1);
        auto map = TypeParam::make(capacity, 0.875);
        for (const Key& key : back) {
            map.insert(key, 1);
        }

        map.insert(near_end[0], 2);
        map.erase(back[0]);
        map.erase(back[1]);
        auto it = map.find(near_end[0]);
        ASSERT_NE(it, map.end());
        ++it;
        ASSERT_EQ(it, map.end());
        set<Key> seen;
        for (auto& [key, value] : map) {
            EXPECT_TRUE(seen.insert(key).second) << "key " << key;
        }

        EXPECT_EQ(seen, (set<Key>{back[2], near_end[0]}));
    }
}

TYPED_TEST(SwissMapBoundaryTest, SmallRequestedCapacity) {
    using Key = typename TypeParam::Key;
    for (size_t requested : {0u, 1u, 8u}) {
        SCOPED_TRACE("requested capacity " + to_string(requested));
        auto map = TypeParam::make(requested, 0.875);
        EXPECT_EQ(map.begin(), map.end());
        EXPECT_EQ(map.find(this->key(0)), map.end());
        for (int i = 0; i < 10; ++i) {
            map.insert(this->key(i), i);
        }

        for (int i = 0; i < 10; ++i) {
            auto it = map.find(this->key(i));
            ASSERT_NE(it, map.end()) << "key " << i;
            EXPECT_EQ(it->second, i);
        }

        for (int i = 0; i < 10; i += 2) {
            map.erase(this->key(i));
        }

        EXPECT_EQ(map.size(), 10u);
        set<Key> seen;
        for (auto& [key, value] : map) {
            EXPECT_TRUE(seen.insert(key).second) << "key " << key;
        }

        set<Key> expected;
        for (int i = 1; i < 10; i += 2) {
            expected.insert(this->key(i));
        }

        EXPECT_EQ(seen, expected);
        for (int i = 10; i < 40; ++i) {
            map.insert(this->key(i), i);
        }

        EXPECT_LT(map.size(), 40u);
        for (int i = 0; i < 40; ++i) {
            auto it = map.find(this->key(i));
            if (i < 10 && i % 2 == 0) {
                EXPECT_EQ(it, map.end()) << "key " << i;
            }

            else {
                ASSERT_NE(it, map.end()) << "key " << i;
                EXPECT_EQ(it->second, i);
                expected.insert(this->key(i));
            }
        }

        seen.clear();
        for (auto& [key, value] : map) {
            EXPECT_TRUE(seen.insert(key).second) << "key " << key;
        }

        EXPECT_EQ(seen, expected);
    }
}

TYPED_TEST(SwissMapBoundaryTest, ManyRehashes) {
    using Key = typename TypeParam::Key;
    auto map = TypeParam::make(16, 0.875);
    for (int i = 0; i < 20000; ++i) {
        map.insert(this->key(i), i * 3);
    }

    EXPECT_EQ(map.size(), 20000u);
    for (int i = 0; i < 20000; ++i) {
        auto it = map.find(this->key(i));
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i * 3);
    }

    for (int i = 20000; i < 20100; ++i) {
        EXPECT_EQ(map.find(this->key(i)), map.end()) << "key " << i;
    }

    set<Key> seen;
    for (auto& [key, value] : map) {
        EXPECT_TRUE(seen.insert(key).second) << "key " << key;
    }

    EXPECT_EQ(seen.size(), 20000u);
}

constexpr array<int, 5> special_keys{-1, -2, 0, 128, 255};

TEST(SwissMapTest, InsertSpecialKeys) {
    Swiss_Map<int, int> map(32, 0.875);
    for (int key : special_keys) {
        auto it = map.insert(key, key * 10);
        ASSERT_NE(it, map.end()) << "key " << key;
        EXPECT_EQ(it->first, key);
        EXPECT_EQ(it->second, key * 10);
    }

    EXPECT_EQ(map.size(), special_keys.size());
}

TEST(SwissMapTest, SubscriptSpecialKeys) {
    Swiss_Map<int, int> map(32, 0.875);
    for (int key : special_keys) {
        EXPECT_EQ(map[key], 0) << "key " << key;
        map[key] = key * 10;
    }

    EXPECT_EQ(map.size(), special_keys.size());
    for (int key : special_keys) {
        EXPECT_EQ(map[key], key * 10) << "key " << key;
    }
}

TEST(SwissMapTest, FindSpecialKeys) {
    Swiss_Map<int, int> map(32, 0.875);
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

TEST(SwissMapTest, EraseSpecialKeys) {
    Swiss_Map<int, int> map(32, 0.875);
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

TEST(SwissMapTest, IterateSpecialKeys) {
    Swiss_Map<int, int> map(32, 0.875);
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

TEST(SwissMapTest, SpecialKeysSurviveRehash) {
    Swiss_Map<int, int> map(4, 0.875);
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
