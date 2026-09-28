#include <gtest/gtest.h>
#include "oa_map.hpp"

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