#include <gtest/gtest.h>
#include <set>
#include <string>
#include "hash.hpp"
#include "map_factory.hpp"

template<typename Map>
size_t live_count(Map& map) {
    size_t count = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        ++count;
    }

    return count;
}

template<typename Factory>
class MapTest : public ::testing::Test {
protected:
    static typename Factory::Key key(int i) {
        return KeyTraits<typename Factory::Key>::key(i);
    }
};

using MapTypes = ::testing::Types<
    OAMapFactory<int>, OAMapFactory<long long>, OAMapFactory<string>,
    ScalarSwissMapFactory<int>, ScalarSwissMapFactory<long long>, ScalarSwissMapFactory<string>>;
TYPED_TEST_SUITE(MapTest, MapTypes, MapTypeNames);

TYPED_TEST(MapTest, StartsEmpty) {
    auto map = TypeParam::make(32, 0.875);
    EXPECT_EQ(map.size(), 0u);
    EXPECT_EQ(map.begin(), map.end());
}

TYPED_TEST(MapTest, InsertSize) {
    auto map = TypeParam::make(32, 0.9);
    EXPECT_EQ(map.size(), 0u);
    map.insert(this->key(3), 5);
    EXPECT_EQ(map.size(), 1u);
    map.insert(this->key(4), 1);
    EXPECT_EQ(map.size(), 2u);
}

TYPED_TEST(MapTest, InsertFind) {
    auto map = TypeParam::make(32, 0.875);
    map.insert(this->key(1), 10);
    auto it = map.find(this->key(1));
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->first, this->key(1));
    EXPECT_EQ(it->second, 10);
}

TYPED_TEST(MapTest, InsertExisting) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(10), 3);
    auto it = map.find(this->key(10));
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->first, this->key(10));
    EXPECT_EQ(it->second, 3);
}

TYPED_TEST(MapTest, InsertDuplicateKeepsSize) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(10), 3);
    map.insert(this->key(10), 7);
    EXPECT_EQ(map.size(), 1u);
}

TYPED_TEST(MapTest, InsertDuplicateKeepsOriginalValue) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(10), 3);
    map.insert(this->key(10), 7);
    auto it = map.find(this->key(10));
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, 3);
}

TYPED_TEST(MapTest, InsertReturnsIteratorToEntry) {
    auto map = TypeParam::make(32, 0.9);
    auto it = map.insert(this->key(10), 3);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->first, this->key(10));
    EXPECT_EQ(it->second, 3);
    EXPECT_EQ(it, map.find(this->key(10)));
}

TYPED_TEST(MapTest, FindEmptyMap) {
    auto map = TypeParam::make(32, 0.9);
    EXPECT_EQ(map.find(this->key(5)), map.end());
}

TYPED_TEST(MapTest, FindMissing) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(1), 1);
    map.insert(this->key(2), 2);
    EXPECT_EQ(map.find(this->key(3)), map.end());
}

TYPED_TEST(MapTest, FindMissingAfterOneInsert) {
    auto map = TypeParam::make(32, 0.875);
    map.insert(this->key(1), 10);
    EXPECT_EQ(map.find(this->key(2)), map.end());
}

TYPED_TEST(MapTest, FindMany) {
    auto map = TypeParam::make(32, 0.9);
    for (int i = 0; i < 20; ++i) {
        map.insert(this->key(i), i * 10);
    }

    for (int i = 0; i < 20; ++i) {
        auto it = map.find(this->key(i));
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i * 10);
    }
}

TYPED_TEST(MapTest, EraseRemovesKey) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(5), 50);
    map.erase(this->key(5));
    EXPECT_EQ(map.find(this->key(5)), map.end());
}

TYPED_TEST(MapTest, EraseKeepsSize) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(5), 50);
    map.insert(this->key(6), 60);
    map.erase(this->key(5));
    EXPECT_EQ(map.size(), 2u);
    EXPECT_EQ(live_count(map), 1u);
}

TYPED_TEST(MapTest, EraseFind) {
    auto map = TypeParam::make(32, 0.875);
    map.insert(this->key(1), 10);
    map.insert(this->key(2), 20);
    map.erase(this->key(1));
    EXPECT_EQ(map.find(this->key(1)), map.end());
    EXPECT_NE(map.find(this->key(2)), map.end());
    EXPECT_EQ(map.size(), 2u);
    EXPECT_EQ(live_count(map), 1u);
}

TYPED_TEST(MapTest, EraseMissing) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(5), 50);
    map.erase(this->key(6));
    EXPECT_EQ(map.size(), 1u);
    EXPECT_NE(map.find(this->key(5)), map.end());
}

TYPED_TEST(MapTest, EraseTwice) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(5), 50);
    map.erase(this->key(5));
    map.erase(this->key(5));
    EXPECT_EQ(map.size(), 1u);
    EXPECT_EQ(live_count(map), 0u);
}

TYPED_TEST(MapTest, ReinsertAfterErase) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(5), 50);
    map.erase(this->key(5));
    map.insert(this->key(5), 70);
    auto it = map.find(this->key(5));
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, 70);
    EXPECT_EQ(map.size(), 2u);
    EXPECT_EQ(live_count(map), 1u);
}

TYPED_TEST(MapTest, FindPastTombstone) {
    const size_t mask = 31;
    SplitMix64<typename TypeParam::Key> hasher;
    int first = 0;
    int second = 1;
    while ((hasher(this->key(second)) & mask) != (hasher(this->key(first)) & mask)) {
        ++second;
    }

    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(first), 1);
    map.insert(this->key(second), 2);
    map.erase(this->key(first));
    auto it = map.find(this->key(second));
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, 2);
}

TYPED_TEST(MapTest, SubscriptCreatesDefault) {
    auto map = TypeParam::make(32, 0.9);
    EXPECT_EQ(map[this->key(5)], 0);
    EXPECT_EQ(map.size(), 1u);
    EXPECT_NE(map.find(this->key(5)), map.end());
}

TYPED_TEST(MapTest, SubscriptReturnsExisting) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(5), 50);
    EXPECT_EQ(map[this->key(5)], 50);
    EXPECT_EQ(map.size(), 1u);
}

TYPED_TEST(MapTest, SubscriptWrite) {
    auto map = TypeParam::make(32, 0.9);
    map[this->key(5)] = 7;
    auto it = map.find(this->key(5));
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, 7);
    map[this->key(5)] = 9;
    it = map.find(this->key(5));
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, 9);
}

TYPED_TEST(MapTest, Subscript) {
    auto map = TypeParam::make(32, 0.875);
    map[this->key(3)] = 30;
    EXPECT_EQ(map[this->key(3)], 30);
    EXPECT_EQ(map[this->key(4)], 0);
    EXPECT_EQ(map.size(), 2u);
}

TYPED_TEST(MapTest, SubscriptAcrossRehash) {
    auto map = TypeParam::make(4, 0.9);
    for (int i = 0; i < 1000; ++i) {
        map[this->key(i)] = i * 2;
    }

    EXPECT_EQ(map.size(), 1000u);
    for (int i = 0; i < 1000; ++i) {
        auto it = map.find(this->key(i));
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i * 2);
    }
}

TYPED_TEST(MapTest, RehashKeepsAllKeys) {
    auto map = TypeParam::make(4, 0.5);
    for (int i = 0; i < 10000; ++i) {
        map.insert(this->key(i), i + 1);
    }

    EXPECT_EQ(map.size(), 10000u);
    for (int i = 0; i < 10000; ++i) {
        auto it = map.find(this->key(i));
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i + 1);
    }
}

TYPED_TEST(MapTest, RehashKeepsAllKeysHighLoad) {
    auto map = TypeParam::make(32, 0.875);
    for (int i = 0; i < 2000; ++i) {
        map.insert(this->key(i), i);
    }

    EXPECT_EQ(map.size(), 2000u);
    for (int i = 0; i < 2000; ++i) {
        auto it = map.find(this->key(i));
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i);
    }
}

TYPED_TEST(MapTest, RehashDropsErased) {
    auto map = TypeParam::make(4, 0.5);
    for (int i = 0; i < 100; ++i) {
        map.insert(this->key(i), i);
    }

    for (int i = 0; i < 100; i += 2) {
        map.erase(this->key(i));
    }

    for (int i = 100; i < 1000; ++i) {
        map.insert(this->key(i), i);
    }

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(map.find(this->key(i)) == map.end(), i % 2 == 0) << "key " << i;
    }

    EXPECT_EQ(live_count(map), 950u);
    EXPECT_GE(map.size(), 950u);
}

TYPED_TEST(MapTest, NonPowerOfTwoCapacity) {
    auto map = TypeParam::make(100, 0.9);
    for (int i = 0; i < 100; ++i) {
        map.insert(this->key(i), i);
    }

    for (int i = 0; i < 100; ++i) {
        EXPECT_NE(map.find(this->key(i)), map.end()) << "key " << i;
    }
}

TYPED_TEST(MapTest, IterateEmpty) {
    auto map = TypeParam::make(32, 0.9);
    EXPECT_EQ(map.begin(), map.end());
}

TYPED_TEST(MapTest, IterateVisitsEachOnce) {
    using Key = typename TypeParam::Key;
    auto map = TypeParam::make(32, 0.9);
    set<Key> expected;
    for (int i = 0; i < 20; ++i) {
        map.insert(this->key(i), i);
        expected.insert(this->key(i));
    }

    multiset<Key> seen;
    for (auto& [key, value] : map) {
        seen.insert(key);
        EXPECT_EQ(key, this->key(value));
    }

    EXPECT_EQ(seen.size(), expected.size());
    EXPECT_EQ(set<Key>(seen.begin(), seen.end()), expected);
}

TYPED_TEST(MapTest, IterateSkipsErased) {
    using Key = typename TypeParam::Key;
    auto map = TypeParam::make(32, 0.9);
    for (int i = 0; i < 20; ++i) {
        map.insert(this->key(i), i);
    }

    for (int i = 0; i < 20; i += 2) {
        map.erase(this->key(i));
    }

    set<Key> seen;
    for (auto& [key, value] : map) {
        seen.insert(key);
    }

    set<Key> expected;
    for (int i = 1; i < 20; i += 2) {
        expected.insert(this->key(i));
    }

    EXPECT_EQ(seen, expected);
}

TYPED_TEST(MapTest, IterateMatchesInserted) {
    using Key = typename TypeParam::Key;
    auto map = TypeParam::make(32, 0.875);
    set<Key> expected;
    for (int i = 0; i < 100; ++i) {
        map.insert(this->key(i), i);
        expected.insert(this->key(i));
    }

    for (int i = 0; i < 100; i += 3) {
        map.erase(this->key(i));
        expected.erase(this->key(i));
    }

    set<Key> seen;
    for (auto& [key, value] : map) {
        EXPECT_TRUE(seen.insert(key).second);
    }

    EXPECT_EQ(seen, expected);
}

TYPED_TEST(MapTest, IterateAllErased) {
    auto map = TypeParam::make(32, 0.9);
    for (int i = 0; i < 10; ++i) {
        map.insert(this->key(i), i);
    }

    for (int i = 0; i < 10; ++i) {
        map.erase(this->key(i));
    }

    EXPECT_EQ(map.begin(), map.end());
}

TYPED_TEST(MapTest, IterateWrite) {
    auto map = TypeParam::make(32, 0.9);
    for (int i = 0; i < 10; ++i) {
        map.insert(this->key(i), i);
    }

    for (auto& [key, value] : map) {
        value *= 100;
    }

    for (int i = 0; i < 10; ++i) {
        auto it = map.find(this->key(i));
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i * 100);
    }
}

TYPED_TEST(MapTest, PostfixIncrement) {
    auto map = TypeParam::make(32, 0.9);
    map.insert(this->key(1), 1);
    map.insert(this->key(2), 2);
    auto it = map.begin();
    auto old = it++;
    EXPECT_NE(old, it);
    EXPECT_NE(old, map.end());
}

TYPED_TEST(MapTest, ChurnSameKey) {
    auto map = TypeParam::make(8, 0.75);
    for (int i = 0; i < 10000; ++i) {
        map.insert(this->key(1), i);
        map.erase(this->key(1));
    }

    EXPECT_EQ(live_count(map), 0u);
    EXPECT_EQ(map.find(this->key(1)), map.end());
}

TYPED_TEST(MapTest, ChurnDistinctKeys) {
    auto map = TypeParam::make(8, 0.75);
    for (int i = 0; i < 10000; ++i) {
        map.insert(this->key(i), i);
        map.erase(this->key(i));
    }

    EXPECT_EQ(live_count(map), 0u);
    EXPECT_EQ(map.find(this->key(42)), map.end());
    EXPECT_EQ(map.begin(), map.end());
}

TYPED_TEST(MapTest, Churn) {
    auto map = TypeParam::make(32, 0.875);
    for (int i = 0; i < 5000; ++i) {
        map.insert(this->key(i), i);
        if (i >= 10) {
            map.erase(this->key(i - 10));
        }
    }

    EXPECT_EQ(live_count(map), 10u);
    for (int i = 4990; i < 5000; ++i) {
        EXPECT_NE(map.find(this->key(i)), map.end()) << "key " << i;
    }

    EXPECT_EQ(map.find(this->key(0)), map.end());
}

TYPED_TEST(MapTest, SizeCountsTombstonesUntilRehash) {
    auto map = TypeParam::make(64, 0.9);
    for (int i = 0; i < 10; ++i) {
        map.insert(this->key(i), i);
    }

    for (int i = 0; i < 5; ++i) {
        map.erase(this->key(i));
    }

    EXPECT_EQ(map.size(), 10u);
    EXPECT_EQ(live_count(map), 5u);
}

TYPED_TEST(MapTest, RehashResetsSizeToLive) {
    auto map = TypeParam::make(8, 0.75);
    map.insert(this->key(5000), 5000);
    for (int i = 0; i < 1000; ++i) {
        map.insert(this->key(i), i);
        map.erase(this->key(i));
    }

    EXPECT_LT(map.size(), 1001u);
    EXPECT_EQ(live_count(map), 1u);
    EXPECT_NE(map.find(this->key(5000)), map.end());
}

TYPED_TEST(MapTest, ChurnMissLookupTerminates) {
    auto map = TypeParam::make(8, 0.75);
    for (int i = 0; i < 1000; ++i) {
        map.insert(this->key(i), i);
        map.erase(this->key(i));
    }

    EXPECT_EQ(map.find(this->key(-5)), map.end());
    EXPECT_EQ(map.find(this->key(1'000'000)), map.end());
}
