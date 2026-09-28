#include <gtest/gtest.h>
#include <set>
#include <string>
#include "oa_map.hpp"
#include "key_traits.hpp"

template<typename Map>
size_t live_count(Map& map) {
    size_t count = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        ++count;
    }

    return count;
}

template<typename K>
class OAMapTyped : public ::testing::Test {
protected:
    OA_Map<K, int> map{32, 0.875, KeyTraits<K>::empty(), KeyTraits<K>::deleted()};
};

using KeyTypes = ::testing::Types<int, long long, string>;
TYPED_TEST_SUITE(OAMapTyped, KeyTypes);

TYPED_TEST(OAMapTyped, StartsEmpty) {
    EXPECT_EQ(this->map.size(), 0);
    EXPECT_EQ(this->map.begin(), this->map.end());
}

TYPED_TEST(OAMapTyped, InsertFind) {
    using Traits = KeyTraits<TypeParam>;
    this->map.insert(Traits::key(1), 10);
    auto it = this->map.find(Traits::key(1));
    ASSERT_NE(it, this->map.end());
    EXPECT_EQ(it->first, Traits::key(1));
    EXPECT_EQ(it->second, 10);
}

TYPED_TEST(OAMapTyped, FindMissing) {
    using Traits = KeyTraits<TypeParam>;
    this->map.insert(Traits::key(1), 10);
    EXPECT_EQ(this->map.find(Traits::key(2)), this->map.end());
}

TYPED_TEST(OAMapTyped, InsertSentinel) {
    using Traits = KeyTraits<TypeParam>;
    EXPECT_THROW(this->map.insert(Traits::empty(), 1), invalid_argument);
    EXPECT_THROW(this->map.insert(Traits::deleted(), 1), invalid_argument);
    EXPECT_EQ(this->map.size(), 0);
}

TYPED_TEST(OAMapTyped, EraseFind) {
    using Traits = KeyTraits<TypeParam>;
    this->map.insert(Traits::key(1), 10);
    this->map.insert(Traits::key(2), 20);
    this->map.erase(Traits::key(1));
    EXPECT_EQ(this->map.find(Traits::key(1)), this->map.end());
    EXPECT_NE(this->map.find(Traits::key(2)), this->map.end());
    EXPECT_EQ(this->map.size(), 2);
    EXPECT_EQ(live_count(this->map), 1);
}

TYPED_TEST(OAMapTyped, Subscript) {
    using Traits = KeyTraits<TypeParam>;
    this->map[Traits::key(3)] = 30;
    EXPECT_EQ(this->map[Traits::key(3)], 30);
    EXPECT_EQ(this->map[Traits::key(4)], 0);
    EXPECT_EQ(this->map.size(), 2);
}

TYPED_TEST(OAMapTyped, RehashKeepsAllKeys) {
    using Traits = KeyTraits<TypeParam>;
    for (int i = 0; i < 2000; ++i) {
        this->map.insert(Traits::key(i), i);
    }

    EXPECT_EQ(this->map.size(), 2000);
    for (int i = 0; i < 2000; ++i) {
        auto it = this->map.find(Traits::key(i));
        ASSERT_NE(it, this->map.end()) << "i = " << i;
        EXPECT_EQ(it->second, i);
    }
}

TYPED_TEST(OAMapTyped, IterateMatchesInserted) {
    using Traits = KeyTraits<TypeParam>;
    set<TypeParam> expected;
    for (int i = 0; i < 100; ++i) {
        this->map.insert(Traits::key(i), i);
        expected.insert(Traits::key(i));
    }

    for (int i = 0; i < 100; i += 3) {
        this->map.erase(Traits::key(i));
        expected.erase(Traits::key(i));
    }

    set<TypeParam> seen;
    for (auto& [key, value] : this->map) {
        EXPECT_TRUE(seen.insert(key).second);
    }

    EXPECT_EQ(seen, expected);
}

TYPED_TEST(OAMapTyped, Churn) {
    using Traits = KeyTraits<TypeParam>;
    for (int i = 0; i < 5000; ++i) {
        this->map.insert(Traits::key(i), i);
        if (i >= 10) {
            this->map.erase(Traits::key(i - 10));
        }
    }

    EXPECT_EQ(live_count(this->map), 10);
    for (int i = 4990; i < 5000; ++i) {
        EXPECT_NE(this->map.find(Traits::key(i)), this->map.end()) << "i = " << i;
    }

    EXPECT_EQ(this->map.find(Traits::key(0)), this->map.end());
}
