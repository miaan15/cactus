#include <gtest/gtest.h>
#include <string>
#include <vector>

import FreelistVector;

using namespace cactus;

// ---------------------------------------------------------------------------
// Insert / Emplace
// ---------------------------------------------------------------------------

TEST(FreelistVector, Insert_SingleElement_ReturnsIndexZero) {
    FreelistVector<int> flv;
    auto index = flv.insert(42);
    EXPECT_EQ(index, 0u);
}

TEST(FreelistVector, Insert_MultipleElements_ReturnsConsecutiveIndices) {
    FreelistVector<int> flv;
    EXPECT_EQ(flv.insert(1), 0u);
    EXPECT_EQ(flv.insert(2), 1u);
    EXPECT_EQ(flv.insert(3), 2u);
}

TEST(FreelistVector, Emplace_WithConstructorArgs_StoresCorrectValue) {
    FreelistVector<std::string> flv;
    auto index = flv.emplace(3u, 'x');
    EXPECT_EQ(flv[index], "xxx");
}

// ---------------------------------------------------------------------------
// Erase
// ---------------------------------------------------------------------------

TEST(FreelistVector, Erase_ValidIndex_MarksSlotInvalid) {
    FreelistVector<int> flv;
    auto index = flv.insert(10);
    flv.erase(index);
    EXPECT_FALSE(flv.contains(index));
}

TEST(FreelistVector, Erase_OutOfBoundsIndex_DoesNothing) {
    FreelistVector<int> flv;
    flv.insert(1);
    EXPECT_NO_FATAL_FAILURE(flv.erase(99u));
}

TEST(FreelistVector, Erase_AlreadyErasedIndex_DoesNothing) {
    FreelistVector<int> flv;
    auto index = flv.insert(5);
    flv.erase(index);
    EXPECT_NO_FATAL_FAILURE(flv.erase(index));
}

// ---------------------------------------------------------------------------
// Freelist reuse
// ---------------------------------------------------------------------------

TEST(FreelistVector, Insert_AfterErase_ReusesFreeSlot) {
    FreelistVector<int> flv;
    flv.insert(1);
    auto freed = flv.insert(2);
    flv.insert(3);

    flv.erase(freed);
    auto reused = flv.insert(99);

    EXPECT_EQ(reused, freed);
    EXPECT_EQ(flv[reused], 99);
}

TEST(FreelistVector, Insert_AfterMultipleErases_ReusesInLifoOrder) {
    FreelistVector<int> flv;
    flv.insert(0); // 0
    flv.insert(1); // 1
    flv.insert(2); // 2

    flv.erase(0u);
    flv.erase(1u);

    // Last erased (1) should be reused first (LIFO freelist)
    auto first_reuse = flv.insert(10);
    auto second_reuse = flv.insert(20);

    EXPECT_EQ(first_reuse, 1u);
    EXPECT_EQ(second_reuse, 0u);
}

// ---------------------------------------------------------------------------
// Access: at(), operator[], contains()
// ---------------------------------------------------------------------------

TEST(FreelistVector, At_ValidIndex_ReturnsPointerToValue) {
    FreelistVector<int> flv;
    auto index = flv.insert(7);
    auto result = flv.at(index);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 7);
}

TEST(FreelistVector, At_OutOfBoundsIndex_ReturnsEmpty) {
    FreelistVector<int> flv;
    flv.insert(1);
    EXPECT_FALSE(flv.at(99u).has_value());
}

TEST(FreelistVector, At_ErasedIndex_ReturnsEmpty) {
    FreelistVector<int> flv;
    auto index = flv.insert(3);
    flv.erase(index);
    EXPECT_FALSE(flv.at(index).has_value());
}

TEST(FreelistVector, At_ConstView_ValidIndex_ReturnsPointerToValue) {
    FreelistVector<int> flv;
    auto index = flv.insert(99);
    const auto &cflv = flv;
    auto result = cflv.at(index);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 99);
}

TEST(FreelistVector, Contains_ValidIndex_ReturnsTrue) {
    FreelistVector<int> flv;
    auto index = flv.insert(1);
    EXPECT_TRUE(flv.contains(index));
}

TEST(FreelistVector, Contains_ErasedIndex_ReturnsFalse) {
    FreelistVector<int> flv;
    auto index = flv.insert(1);
    flv.erase(index);
    EXPECT_FALSE(flv.contains(index));
}

TEST(FreelistVector, Contains_OutOfBoundsIndex_ReturnsFalse) {
    FreelistVector<int> flv;
    EXPECT_FALSE(flv.contains(0u));
}

// ---------------------------------------------------------------------------
// Iteration
// ---------------------------------------------------------------------------

TEST(FreelistVector, Iterator_ForwardIteration_SkipsErasedSlots) {
    FreelistVector<int> flv;
    flv.insert(10); // 0
    flv.insert(20); // 1
    flv.insert(30); // 2
    flv.erase(1u);

    std::vector<int> result;
    for (auto &v : flv) result.push_back(v);

    EXPECT_EQ(result, (std::vector<int>{10, 30}));
}

TEST(FreelistVector, Iterator_EmptyContainer_BeginEqualsEnd) {
    FreelistVector<int> flv;
    EXPECT_EQ(flv.begin(), flv.end());
}

TEST(FreelistVector, Iterator_AllElementsErased_BeginEqualsEnd) {
    FreelistVector<int> flv;
    auto a = flv.insert(1);
    auto b = flv.insert(2);
    flv.erase(a);
    flv.erase(b);
    EXPECT_EQ(flv.begin(), flv.end());
}

TEST(FreelistVector, ReverseIterator_ReturnsElementsInReverseOrder) {
    FreelistVector<int> flv;
    flv.insert(1); // 0
    flv.insert(2); // 1
    flv.insert(3); // 2
    flv.erase(1u);

    std::vector<int> result(flv.rbegin(), flv.rend());
    EXPECT_EQ(result, (std::vector<int>{3, 1}));
}

// ---------------------------------------------------------------------------
// Size / Capacity / Reserve
// ---------------------------------------------------------------------------

TEST(FreelistVector, Size_AfterInserts_ReflectsUnderlyingStorageSize) {
    FreelistVector<int> flv;
    flv.insert(1);
    flv.insert(2);
    // size() counts all slots, not just live ones
    EXPECT_EQ(flv.size(), 2u);
}

TEST(FreelistVector, Size_AfterErase_DoesNotShrink) {
    FreelistVector<int> flv;
    auto index = flv.insert(1);
    flv.insert(2);
    flv.erase(index);
    EXPECT_EQ(flv.size(), 2u);
}

TEST(FreelistVector, Reserve_SetsCapacityAtLeastToRequestedSize) {
    FreelistVector<int> flv;
    flv.reserve(100u);
    EXPECT_GE(flv.capacity(), 100u);
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(FreelistVector, Clear_NonEmptyContainer_BecomesEmpty) {
    FreelistVector<int> flv;
    flv.insert(1);
    flv.insert(2);
    flv.clear();
    EXPECT_EQ(flv.size(), 0u);
    EXPECT_EQ(flv.begin(), flv.end());
}

TEST(FreelistVector, Clear_ThenInsert_StartsFromIndexZero) {
    FreelistVector<int> flv;
    flv.insert(1);
    flv.insert(2);
    flv.clear();
    auto index = flv.insert(99);
    EXPECT_EQ(index, 0u);
    EXPECT_EQ(flv[index], 99);
}

// ---------------------------------------------------------------------------
// Swap
// ---------------------------------------------------------------------------

TEST(FreelistVector, Swap_TwoContainers_ExchangesContents) {
    FreelistVector<int> a, b;
    a.insert(1);
    a.insert(2);
    b.insert(99);

    a.swap(b);

    EXPECT_EQ(a.size(), 1u);
    EXPECT_EQ(a[0], 99);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
}

TEST(FreelistVector, Swap_FreeFunction_ExchangesContents) {
    FreelistVector<int> a, b;
    a.insert(10);
    b.insert(20);
    b.insert(30);

    swap(a, b);

    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a[0], 20);
    EXPECT_EQ(a[1], 30);
    EXPECT_EQ(b.size(), 1u);
    EXPECT_EQ(b[0], 10);
}

// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
