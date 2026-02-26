#include <gtest/gtest.h>
#include <string>
#include <vector>

import SlotMap;

using namespace cactus;

// ---------------------------------------------------------------------------
// Key helpers
// ---------------------------------------------------------------------------

TEST(SlotMapKeyHelpers, GetIdx_ReturnsUpperBits) {
    uint64_t key = (uint64_t{5} << 32) | 3;
    EXPECT_EQ(get_idx(key), 5u);
}

TEST(SlotMapKeyHelpers, GetGen_ReturnsLowerBits) {
    uint64_t key = (uint64_t{5} << 32) | 3;
    EXPECT_EQ(get_gen(key), 3u);
}

TEST(SlotMapKeyHelpers, SetIdx_UpdatesUpperBitsOnly) {
    uint64_t key = (uint64_t{1} << 32) | 7;
    set_idx(&key, 99u);
    EXPECT_EQ(get_idx(key), 99u);
    EXPECT_EQ(get_gen(key), 7u); // generation preserved
}

TEST(SlotMapKeyHelpers, IncreaseGen_IncrementsGenerationOnly) {
    uint64_t key = (uint64_t{4} << 32) | 2;
    increase_gen(&key);
    EXPECT_EQ(get_gen(key), 3u);
    EXPECT_EQ(get_idx(key), 4u); // index preserved
}

// ---------------------------------------------------------------------------
// Insert / Emplace
// ---------------------------------------------------------------------------

TEST(SlotMap, Insert_SingleElement_ReturnsValidKey) {
    SlotMap<int> sm;
    auto key = sm.insert(42);
    EXPECT_EQ(get_idx(key), 0u);
    EXPECT_EQ(get_gen(key), 0u);
    EXPECT_EQ(sm.size(), 1u);
}

TEST(SlotMap, Insert_MultipleElements_SizeGrowsAccordingly) {
    SlotMap<int> sm;
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);
    EXPECT_EQ(sm.size(), 3u);
}

TEST(SlotMap, Emplace_WithConstructorArgs_StoresCorrectValue) {
    SlotMap<std::string> sm;
    auto key = sm.emplace("hello");
    EXPECT_EQ(*sm.at(key).value(), "hello");
}

// ---------------------------------------------------------------------------
// Find / At / operator[]
// ---------------------------------------------------------------------------

TEST(SlotMap, Find_ValidKey_ReturnsCorrectIterator) {
    SlotMap<int> sm;
    auto key = sm.insert(7);
    auto it = sm.find(key);
    ASSERT_NE(it, sm.end());
    EXPECT_EQ(*it, 7);
}

TEST(SlotMap, Find_InvalidKey_ReturnsEnd) {
    SlotMap<int> sm;
    sm.insert(1);
    uint64_t bad_key = (uint64_t{99} << 32) | 0;
    EXPECT_EQ(sm.find(bad_key), sm.end());
}

TEST(SlotMap, At_ValidKey_ReturnsPointerToValue) {
    SlotMap<int> sm;
    auto key = sm.insert(55);
    auto result = sm.at(key);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 55);
}

TEST(SlotMap, At_InvalidKey_ReturnsEmpty) {
    SlotMap<int> sm;
    uint64_t bad_key = (uint64_t{99} << 32) | 0;
    EXPECT_FALSE(sm.at(bad_key).has_value());
}

TEST(SlotMap, At_ConstView_ValidKey_ReturnsPointerToValue) {
    SlotMap<int> sm;
    auto key = sm.insert(77);
    const auto &csm = sm;
    auto result = csm.at(key);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 77);
}

TEST(SlotMap, SubscriptOperator_ValidKey_ReturnsCorrectValue) {
    SlotMap<int> sm;
    auto key = sm.insert(33);
    EXPECT_EQ(sm[key], 33);
}

// ---------------------------------------------------------------------------
// Erase (by key)
// ---------------------------------------------------------------------------

TEST(SlotMap, Erase_ByKey_RemovesElement) {
    SlotMap<int> sm;
    auto key = sm.insert(10);
    sm.erase(key);
    EXPECT_EQ(sm.size(), 0u);
    EXPECT_FALSE(sm.at(key).has_value());
}

TEST(SlotMap, Erase_ByKey_InvalidatesKey) {
    SlotMap<int> sm;
    auto key = sm.insert(10);
    sm.erase(key);
    // generation bumped — stale key should not resolve
    EXPECT_EQ(sm.find(key), sm.end());
}

TEST(SlotMap, Erase_ByKey_StaleKey_ReturnsEnd) {
    SlotMap<int> sm;
    auto key = sm.insert(1);
    sm.erase(key);
    auto result = sm.erase(key); // second erase with stale key
    EXPECT_EQ(result, sm.end());
}

TEST(SlotMap, Erase_ByKey_PreservesOtherElements) {
    SlotMap<int> sm;
    auto k1 = sm.insert(1);
    auto k2 = sm.insert(2);
    auto k3 = sm.insert(3);
    sm.erase(k2);
    EXPECT_TRUE(sm.at(k1).has_value());
    EXPECT_FALSE(sm.at(k2).has_value());
    EXPECT_TRUE(sm.at(k3).has_value());
}

// ---------------------------------------------------------------------------
// Slot reuse after erase
// ---------------------------------------------------------------------------

TEST(SlotMap, Insert_AfterErase_ReusesFreeSlot) {
    SlotMap<int> sm;
    auto k1 = sm.insert(1);
    sm.erase(k1);
    auto k2 = sm.insert(2);

    // slot index should be reused, but generation must differ
    EXPECT_EQ(get_idx(k1), get_idx(k2));
    EXPECT_NE(get_gen(k1), get_gen(k2));
    EXPECT_EQ(sm[k2], 2);
}

TEST(SlotMap, Insert_AfterErase_OldKeyStillInvalid) {
    SlotMap<int> sm;
    auto old_key = sm.insert(42);
    sm.erase(old_key);
    sm.insert(99); // reuses same slot

    EXPECT_FALSE(sm.at(old_key).has_value());
}

// ---------------------------------------------------------------------------
// Erase (by iterator range)
// ---------------------------------------------------------------------------

TEST(SlotMap, Erase_IteratorRange_RemovesAllInRange) {
    SlotMap<int> sm;
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);
    sm.erase(sm.begin(), sm.end());
    EXPECT_TRUE(sm.empty());
}

// ---------------------------------------------------------------------------
// Iteration
// ---------------------------------------------------------------------------

TEST(SlotMap, Iterator_ForwardIteration_VisitsAllLiveElements) {
    SlotMap<int> sm;
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    std::vector<int> result(sm.begin(), sm.end());
    std::sort(result.begin(), result.end());
    EXPECT_EQ(result, (std::vector<int>{10, 20, 30}));
}

TEST(SlotMap, Iterator_EmptyContainer_BeginEqualsEnd) {
    SlotMap<int> sm;
    EXPECT_EQ(sm.begin(), sm.end());
}

TEST(SlotMap, ReverseIterator_ReturnsElementsInReverseOrder) {
    SlotMap<int> sm;
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);

    std::vector<int> result(sm.rbegin(), sm.rend());
    EXPECT_EQ(result, (std::vector<int>{3, 2, 1}));
}

// ---------------------------------------------------------------------------
// Size / Empty / Capacity / Reserve
// ---------------------------------------------------------------------------

TEST(SlotMap, Empty_NewContainer_ReturnsTrue) {
    SlotMap<int> sm;
    EXPECT_TRUE(sm.empty());
}

TEST(SlotMap, Empty_AfterInsert_ReturnsFalse) {
    SlotMap<int> sm;
    sm.insert(1);
    EXPECT_FALSE(sm.empty());
}

TEST(SlotMap, Empty_AfterInsertAndErase_ReturnsTrue) {
    SlotMap<int> sm;
    auto key = sm.insert(1);
    sm.erase(key);
    EXPECT_TRUE(sm.empty());
}

TEST(SlotMap, Reserve_SetsCapacityAtLeastToRequestedSize) {
    SlotMap<int> sm;
    sm.reserve(64u);
    EXPECT_GE(sm.capacity(), 64u);
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(SlotMap, Clear_NonEmptyContainer_BecomesEmpty) {
    SlotMap<int> sm;
    sm.insert(1);
    sm.insert(2);
    sm.clear();
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.begin(), sm.end());
}

TEST(SlotMap, Clear_ThenInsert_WorksCorrectly) {
    SlotMap<int> sm;
    sm.insert(1);
    sm.clear();
    auto key = sm.insert(99);
    EXPECT_EQ(sm[key], 99);
    EXPECT_EQ(sm.size(), 1u);
}

// ---------------------------------------------------------------------------
// Swap
// ---------------------------------------------------------------------------

TEST(SlotMap, Swap_MemberFunction_ExchangesContents) {
    SlotMap<int> a, b;
    auto ka = a.insert(1);
    b.insert(10);
    b.insert(20);

    a.swap(b);

    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(b.size(), 1u);
    EXPECT_EQ(b[ka], 1);
}

TEST(SlotMap, Swap_FreeFunction_ExchangesContents) {
    SlotMap<int> a, b;
    auto ka = a.insert(42);
    b.insert(7);

    swap(a, b);

    EXPECT_EQ(a.size(), 1u);
    EXPECT_EQ(a[ka], 7); // ka's slot index reused in a, value now 7...
    // verify via size and direct lookup on b
    EXPECT_EQ(b.size(), 1u);
    EXPECT_EQ(b[ka], 42);
}

// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
