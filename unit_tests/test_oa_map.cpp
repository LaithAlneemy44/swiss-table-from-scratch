#include <gtest/gtest.h>
#include "oa_map.hpp"

TEST(OAMapTest, InsertSentinel) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    EXPECT_THROW(map.insert(-1, 2), invalid_argument);
    EXPECT_THROW(map.insert(-2, 2), invalid_argument);
}

TEST(OAMapTest, InsertSentinelLeavesMapUnchanged) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(1, 1);
    EXPECT_THROW(map.insert(-1, 2), invalid_argument);
    EXPECT_EQ(map.size(), 1u);
    EXPECT_EQ(map.find(-1), map.end());
}

TEST(OAMapTest, FindSentinel) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(1, 1);
    EXPECT_EQ(map.find(-1), map.end());
    EXPECT_EQ(map.find(-2), map.end());
}

TEST(OAMapTest, EraseSentinel) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    map.insert(5, 50);
    EXPECT_NO_THROW(map.erase(-1));
    EXPECT_NO_THROW(map.erase(-2));
    EXPECT_EQ(map.size(), 1u);
}

TEST(OAMapTest, SubscriptSentinel) {
    OA_Map<int, int> map(32, 0.9, -1, -2);
    EXPECT_THROW(map[-1], invalid_argument);
    EXPECT_THROW(map[-2], invalid_argument);
}
