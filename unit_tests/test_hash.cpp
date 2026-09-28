#include <gtest/gtest.h>
#include <bit>
#include <set>
#include <string>
#include <vector>
#include "hash.hpp"

TEST(HashTest, Deterministic) {
    SplitMix64<int> hasher;
    EXPECT_EQ(hasher(42), hasher(42));
    EXPECT_EQ(SplitMix64<string>{}("abc"), SplitMix64<string>{}("abc"));
}

TEST(HashTest, ZeroNotZero) {
    EXPECT_NE(SplitMix64<int>{}(0), 0);
}

TEST(HashTest, NoCollisionsSequential) {
    SplitMix64<int> hasher;
    set<size_t> seen;
    for (int i = 0; i < 100000; ++i) {
        EXPECT_TRUE(seen.insert(hasher(i)).second) << "key " << i;
    }
}

TEST(HashTest, SequentialKeysSpreadAcrossSlots) {
    SplitMix64<int> hasher;
    const size_t slots = 1024;
    vector<int> counts(slots, 0);
    for (int i = 0; i < static_cast<int>(slots) * 8; ++i) {
        ++counts[hasher(i) & (slots - 1)];
    }

    int empty = 0;
    for (int c : counts) {
        if (c == 0) {
            ++empty;
        }
    }

    EXPECT_LT(empty, 10);
}

TEST(HashTest, StridedKeysSpreadAcrossSlots) {
    SplitMix64<int> hasher;
    const size_t slots = 1024;
    set<size_t> used;
    for (int i = 0; i < 1024; ++i) {
        used.insert(hasher(i * 1024) & (slots - 1));
    }

    EXPECT_GT(used.size(), 500);
}

TEST(HashTest, SingleBitFlipChangesManyBits) {
    SplitMix64<uint64_t> hasher;
    for (uint64_t x : {0ULL, 1ULL, 12345ULL, 0xdeadbeefULL}) {
        for (int bit = 0; bit < 64; ++bit) {
            const uint64_t flipped = x ^ (1ULL << bit);
            const int changed = popcount(static_cast<uint64_t>(hasher(x) ^ hasher(flipped)));
            EXPECT_GT(changed, 10) << "x " << x << ", bit " << bit;
            EXPECT_LT(changed, 54) << "x " << x << ", bit " << bit;
        }
    }
}
