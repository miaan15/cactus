#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

import SlotMap;

using cactus::get_gen;
using cactus::get_idx;
using cactus::SlotMap;

// ---------------------------------------------------------------------------
// Helper key utilities
// ---------------------------------------------------------------------------

TEST(SlotMapKeyHelpers, GetIdxReturnsUpper32Bits) {
    uint64_t key = 0x0000'0003'0000'0005ULL;
    EXPECT_EQ(get_idx(key), 3u);
}

TEST(SlotMapKeyHelpers, GetGenReturnsLower32Bits) {
    uint64_t key = 0x0000'0003'0000'0005ULL;
    EXPECT_EQ(get_gen(key), 5u);
}

// ---------------------------------------------------------------------------
// Construction & empty state
// ---------------------------------------------------------------------------

TEST(SlotMapTest, DefaultConstructedIsEmpty) {
    SlotMap<int> sm{};
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
}

// ---------------------------------------------------------------------------
// Insert / find / at
// ---------------------------------------------------------------------------

TEST(SlotMapTest, InsertSingleElement) {
    SlotMap<int> sm{};
    auto key = sm.insert(42);

    EXPECT_EQ(sm.size(), 1u);
    EXPECT_FALSE(sm.empty());

    auto it = sm.find(key);
    ASSERT_NE(it, sm.end());
    EXPECT_EQ(*it, 42);
}

TEST(SlotMapTest, InsertMultipleElements) {
    SlotMap<std::string> sm{};
    auto k1 = sm.insert("alpha");
    auto k2 = sm.insert("beta");
    auto k3 = sm.insert("gamma");

    EXPECT_EQ(sm.size(), 3u);

    EXPECT_EQ(*sm.find(k1), "alpha");
    EXPECT_EQ(*sm.find(k2), "beta");
    EXPECT_EQ(*sm.find(k3), "gamma");
}

TEST(SlotMapTest, AtReturnsValueWhenKeyValid) {
    SlotMap<int> sm{};
    auto key = sm.insert(99);

    auto opt = sm.at(key);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(*opt.value(), 99);
}

TEST(SlotMapTest, AtReturnsNulloptWhenKeyInvalid) {
    SlotMap<int> sm{};
    sm.insert(10);

    uint64_t bogus_key = 0xDEAD'BEEF'0000'00FFULL;
    EXPECT_FALSE(sm.at(bogus_key).has_value());
}

TEST(SlotMapTest, FindReturnsEndForOutOfRangeIndex) {
    SlotMap<int> sm{};
    sm.insert(1);

    uint64_t bad_key = 0x0000'0000'0000'00FFULL; // sparse idx way out of range
    EXPECT_EQ(sm.find(bad_key), sm.end());
}

// ---------------------------------------------------------------------------
// Erase by key
// ---------------------------------------------------------------------------

TEST(SlotMapTest, EraseByKeyRemovesElement) {
    SlotMap<int> sm{};
    auto k1 = sm.insert(10);
    auto k2 = sm.insert(20);
    auto k3 = sm.insert(30);

    EXPECT_NE(sm.erase(k2), sm.end());
    EXPECT_EQ(sm.size(), 2u);
    EXPECT_EQ(sm.find(k2), sm.end());

    // remaining elements still reachable
    EXPECT_NE(sm.find(k1), sm.end());
    EXPECT_NE(sm.find(k3), sm.end());
}

TEST(SlotMapTest, EraseByInvalidKeyReturnsZero) {
    SlotMap<int> sm{};
    sm.insert(1);

    uint64_t bogus = 0xBAD0'0000'0000'0000ULL;
    EXPECT_EQ(sm.erase(bogus), sm.end());
}

TEST(SlotMapTest, ErasedKeyIsInvalidatedByGeneration) {
    SlotMap<int> sm{};
    auto key = sm.insert(42);
    sm.erase(key);

    // same slot may be reused, but the old key must not resolve
    EXPECT_EQ(sm.find(key), sm.end());
    EXPECT_FALSE(sm.at(key).has_value());
}

// ---------------------------------------------------------------------------
// Erase by iterator
// ---------------------------------------------------------------------------

TEST(SlotMapTest, EraseByIterator) {
    SlotMap<int> sm{};
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);

    auto it = sm.begin();
    sm.erase(it);

    EXPECT_EQ(sm.size(), 2u);
}

// ---------------------------------------------------------------------------
// Slot reuse after erase
// ---------------------------------------------------------------------------

TEST(SlotMapTest, SlotIsReusedAfterErase) {
    SlotMap<int> sm{};
    auto k1 = sm.insert(100);
    sm.erase(k1);

    auto k2 = sm.insert(200);
    EXPECT_EQ(sm.size(), 1u);
    EXPECT_EQ(*sm.find(k2), 200);

    // old key must still be invalid
    EXPECT_EQ(sm.find(k1), sm.end());
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(SlotMapTest, ClearRemovesAllElements) {
    SlotMap<int> sm{};
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);

    sm.clear();

    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
    EXPECT_EQ(sm.begin(), sm.end());
}

// ---------------------------------------------------------------------------
// Reserve
// ---------------------------------------------------------------------------

TEST(SlotMapTest, ReserveDoesNotChangeSize) {
    SlotMap<int> sm{};
    sm.reserve(100);
    EXPECT_TRUE(sm.empty());
}

// ---------------------------------------------------------------------------
// Iteration
// ---------------------------------------------------------------------------

TEST(SlotMapTest, RangeBasedForVisitsAllElements) {
    SlotMap<int> sm{};
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    int sum = 0;
    for (auto &v : sm) {
        sum += v;
    }
    EXPECT_EQ(sum, 60);
}

TEST(SlotMapTest, ReverseIteratorsWork) {
    SlotMap<int> sm{};
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);

    std::vector<int> reversed(sm.rbegin(), sm.rend());
    ASSERT_EQ(reversed.size(), 3u);
    // reverse of insertion order
    EXPECT_EQ(reversed[0], 3);
    EXPECT_EQ(reversed[1], 2);
    EXPECT_EQ(reversed[2], 1);
}

// ---------------------------------------------------------------------------
// Swap
// ---------------------------------------------------------------------------

TEST(SlotMapTest, SwapExchangesContents) {
    SlotMap<int> a{};
    SlotMap<int> b{};

    auto ka = a.insert(1);
    b.insert(100);
    b.insert(200);

    swap(a, b);

    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(b.size(), 1u);
    EXPECT_NE(b.find(ka), b.end());
}

// ---------------------------------------------------------------------------
// Const access
// ---------------------------------------------------------------------------

TEST(SlotMapTest, ConstFindAndAt) {
    SlotMap<int> sm{};
    auto key = sm.insert(77);

    const auto &csm = sm;

    auto it = csm.find(key);
    ASSERT_NE(it, csm.end());
    EXPECT_EQ(*it, 77);

    auto opt = csm.at(key);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(*opt.value(), 77);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
