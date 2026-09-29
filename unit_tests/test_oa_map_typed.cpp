#include <gtest/gtest.h>
#include <string>
#include "oa_map.hpp"
#include "key_traits.hpp"

template<typename K>
class OAMapTyped : public ::testing::Test {
protected:
    OA_Map<K, int> map{32, 0.875, KeyTraits<K>::empty(), KeyTraits<K>::deleted()};
};

using KeyTypes = ::testing::Types<int, long long, string>;
TYPED_TEST_SUITE(OAMapTyped, KeyTypes);

TYPED_TEST(OAMapTyped, InsertSentinel) {
    using Traits = KeyTraits<TypeParam>;
    EXPECT_THROW(this->map.insert(Traits::empty(), 1), invalid_argument);
    EXPECT_THROW(this->map.insert(Traits::deleted(), 1), invalid_argument);
    EXPECT_EQ(this->map.size(), 0u);
}
