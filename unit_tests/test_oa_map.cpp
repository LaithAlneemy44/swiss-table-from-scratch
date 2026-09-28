#include <gtest/gtest.h>
#include <set>
#include <vector>
#include "oa_map.hpp"

template<typename Map>
size_t live_count(Map& map) {
    size_t count = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        ++count;
    }

    return count;
}

TEST(OAMapTest, InsertSize) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    EXPECT_EQ(map.size(), 0);
    map.insert(3, 5);
    EXPECT_EQ(map.size(), 1);
    map.insert(4, 1);
    EXPECT_EQ(map.size(), 2);
}

TEST(OAMapTest, InsertSentinel) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    EXPECT_THROW(map.insert(-1, 2), invalid_argument);
    EXPECT_THROW(map.insert(-2, 2), invalid_argument);
}

TEST(OAMapTest, InsertExisting) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(10, 3);
    auto it = map.find(10);
    EXPECT_EQ(it->first, 10);
    EXPECT_EQ(it->second, 3);
}

TEST(OAMapTest, InsertDuplicateKeepsSize) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(10, 3);
    map.insert(10, 7);
    EXPECT_EQ(map.size(), 1);
}

TEST(OAMapTest, InsertDuplicateKeepsOriginalValue) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(10, 3);
    map.insert(10, 7);
    EXPECT_EQ(map.find(10)->second, 3);
}

TEST(OAMapTest, InsertReturnsIteratorToEntry) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    auto it = map.insert(10, 3);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->first, 10);
    EXPECT_EQ(it->second, 3);
    EXPECT_EQ(it, map.find(10));
}

TEST(OAMapTest, InsertSentinelLeavesMapUnchanged) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(1, 1);
    EXPECT_THROW(map.insert(-1, 2), invalid_argument);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map.find(-1), map.end());
}

TEST(OAMapTest, FindEmptyMap) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    EXPECT_EQ(map.find(5), map.end());
}

TEST(OAMapTest, FindMissing) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(1, 1);
    map.insert(2, 2);
    EXPECT_EQ(map.find(3), map.end());
}

TEST(OAMapTest, FindSentinel) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(1, 1);
    EXPECT_EQ(map.find(-1), map.end());
    EXPECT_EQ(map.find(-2), map.end());
}

TEST(OAMapTest, FindMany) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    for (int i = 0; i < 20; ++i) {
        map.insert(i, i * 10);
    }

    for (int i = 0; i < 20; ++i) {
        auto it = map.find(i);
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i * 10);
    }
}

TEST(OAMapTest, EraseRemovesKey) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(5, 50);
    map.erase(5);
    EXPECT_EQ(map.find(5), map.end());
}

TEST(OAMapTest, EraseKeepsSize) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(5, 50);
    map.insert(6, 60);
    map.erase(5);
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(live_count(map), 1);
}

TEST(OAMapTest, EraseMissing) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(5, 50);
    map.erase(6);
    EXPECT_EQ(map.size(), 1);
    EXPECT_NE(map.find(5), map.end());
}

TEST(OAMapTest, EraseSentinel) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(5, 50);
    EXPECT_NO_THROW(map.erase(-1));
    EXPECT_NO_THROW(map.erase(-2));
    EXPECT_EQ(map.size(), 1);
}

TEST(OAMapTest, EraseTwice) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(5, 50);
    map.erase(5);
    map.erase(5);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(live_count(map), 0);
}

TEST(OAMapTest, ReinsertAfterErase) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(5, 50);
    map.erase(5);
    map.insert(5, 70);
    ASSERT_NE(map.find(5), map.end());
    EXPECT_EQ(map.find(5)->second, 70);
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(live_count(map), 1);
}

TEST(OAMapTest, FindPastTombstone) {
    const size_t mask = 31;
    SplitMix64<int> hasher;
    int first = 0;
    int second = 1;
    while ((hasher(second) & mask) != (hasher(first) & mask)) {
        ++second;
    }

    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(first, 1);
    map.insert(second, 2);
    map.erase(first);
    auto it = map.find(second);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, 2);
}

TEST(OAMapTest, SubscriptCreatesDefault) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    EXPECT_EQ(map[5], 0);
    EXPECT_EQ(map.size(), 1);
    EXPECT_NE(map.find(5), map.end());
}

TEST(OAMapTest, SubscriptReturnsExisting) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(5, 50);
    EXPECT_EQ(map[5], 50);
    EXPECT_EQ(map.size(), 1);
}

TEST(OAMapTest, SubscriptWrite) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map[5] = 7;
    EXPECT_EQ(map.find(5)->second, 7);
    map[5] = 9;
    EXPECT_EQ(map.find(5)->second, 9);
}

TEST(OAMapTest, SubscriptSentinel) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    EXPECT_THROW(map[-1], invalid_argument);
    EXPECT_THROW(map[-2], invalid_argument);
}

TEST(OAMapTest, SubscriptAcrossRehash) {
    OA_Map<int, int> map(4, 0.9, -1, -2);
    for (int i = 0; i < 1000; ++i) {
        map[i] = i * 2;
    }

    EXPECT_EQ(map.size(), 1000);
    for (int i = 0; i < 1000; ++i) {
        auto it = map.find(i);
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i * 2);
    }
}

TEST(OAMapTest, RehashKeepsAllKeys) {
    OA_Map<int, int> map(4, 0.5, -1, -2);
    for (int i = 0; i < 10000; ++i) {
        map.insert(i, i + 1);
    }

    EXPECT_EQ(map.size(), 10000);
    for (int i = 0; i < 10000; ++i) {
        auto it = map.find(i);
        ASSERT_NE(it, map.end()) << "key " << i;
        EXPECT_EQ(it->second, i + 1);
    }
}

TEST(OAMapTest, RehashDropsErased) {
    OA_Map<int, int> map(4, 0.5, -1, -2);
    for (int i = 0; i < 100; ++i) {
        map.insert(i, i);
    }

    for (int i = 0; i < 100; i += 2) {
        map.erase(i);
    }

    for (int i = 100; i < 1000; ++i) {
        map.insert(i, i);
    }

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(map.find(i) == map.end(), i % 2 == 0) << "key " << i;
    }

    EXPECT_EQ(live_count(map), 950);
    EXPECT_GE(map.size(), 950);
}

TEST(OAMapTest, NonPowerOfTwoCapacity) {
    OA_Map<int, int> map(100, 0.9, -1, -2);
    for (int i = 0; i < 100; ++i) {
        map.insert(i, i);
    }

    for (int i = 0; i < 100; ++i) {
        EXPECT_NE(map.find(i), map.end()) << "key " << i;
    }
}

TEST(OAMapTest, IterateEmpty) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    EXPECT_EQ(map.begin(), map.end());
}

TEST(OAMapTest, IterateVisitsEachOnce) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    set<int> expected;
    for (int i = 0; i < 20; ++i) {
        map.insert(i, i);
        expected.insert(i);
    }

    multiset<int> seen;
    for (auto& [key, value] : map) {
        seen.insert(key);
        EXPECT_EQ(key, value);
    }

    EXPECT_EQ(seen.size(), expected.size());
    EXPECT_EQ(set<int>(seen.begin(), seen.end()), expected);
}

TEST(OAMapTest, IterateSkipsErased) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    for (int i = 0; i < 20; ++i) {
        map.insert(i, i);
    }

    for (int i = 0; i < 20; i += 2) {
        map.erase(i);
    }

    set<int> seen;
    for (auto& [key, value] : map) {
        seen.insert(key);
    }

    set<int> expected;
    for (int i = 1; i < 20; i += 2) {
        expected.insert(i);
    }

    EXPECT_EQ(seen, expected);
}

TEST(OAMapTest, IterateAllErased) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    for (int i = 0; i < 10; ++i) {
        map.insert(i, i);
    }

    for (int i = 0; i < 10; ++i) {
        map.erase(i);
    }

    EXPECT_EQ(map.begin(), map.end());
}

TEST(OAMapTest, IterateWrite) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    for (int i = 0; i < 10; ++i) {
        map.insert(i, i);
    }

    for (auto& [key, value] : map) {
        value = key * 100;
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(map.find(i)->second, i * 100);
    }
}

TEST(OAMapTest, PostfixIncrement) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(1, 1);
    map.insert(2, 2);
    auto it = map.begin();
    auto old = it++;
    EXPECT_NE(old, it);
    EXPECT_NE(old, map.end());
}

TEST(OAMapTest, ChurnSameKey) {
    OA_Map<int, int> map(8, 0.75, -1, -2);
    for (int i = 0; i < 10000; ++i) {
        map.insert(1, i);
        map.erase(1);
    }

    EXPECT_EQ(live_count(map), 0);
    EXPECT_EQ(map.find(1), map.end());
}

TEST(OAMapTest, ChurnDistinctKeys) {
    OA_Map<int, int> map(8, 0.75, -1, -2);
    for (int i = 0; i < 10000; ++i) {
        map.insert(i, i);
        map.erase(i);
    }

    EXPECT_EQ(live_count(map), 0);
    EXPECT_EQ(map.find(42), map.end());
    EXPECT_EQ(map.begin(), map.end());
}

TEST(OAMapTest, SizeCountsTombstonesUntilRehash) {
    OA_Map<int, int> map(64, 0.9, -1, -2);
    for (int i = 0; i < 10; ++i) {
        map.insert(i, i);
    }

    for (int i = 0; i < 5; ++i) {
        map.erase(i);
    }

    EXPECT_EQ(map.size(), 10);
    EXPECT_EQ(live_count(map), 5);
}

TEST(OAMapTest, RehashResetsSizeToLive) {
    OA_Map<int, int> map(8, 0.75, -1, -2);
    map.insert(5000, 5000);
    for (int i = 0; i < 1000; ++i) {
        map.insert(i, i);
        map.erase(i);
    }

    EXPECT_LT(map.size(), 1001);
    EXPECT_EQ(live_count(map), 1);
    EXPECT_NE(map.find(5000), map.end());
}

TEST(OAMapTest, ChurnMissLookupTerminates) {
    OA_Map<int, int> map(8, 0.75, -1, -2);
    for (int i = 0; i < 1000; ++i) {
        map.insert(i, i);
        map.erase(i);
    }

    EXPECT_EQ(map.find(-5), map.end());
    EXPECT_EQ(map.find(1'000'000), map.end());
}
