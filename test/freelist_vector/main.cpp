#include <gtest/gtest.h>
#include <vector>

import FreelistVector;

using cactus::FreelistVector;

// ---------------------------------------------------------------------------
// Construction & empty state
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, DefaultConstructedIsEmpty) {
    FreelistVector<int> fv{};
    EXPECT_EQ(fv.begin(), fv.end());
}

// ---------------------------------------------------------------------------
// Insert / at / operator[]
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, InsertSingleElement) {
    FreelistVector<int> fv{};
    auto idx = fv.insert(42);

    auto opt = fv.at(idx);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(**opt, 42);
}

TEST(FreelistVectorTest, InsertMultipleElements) {
    FreelistVector<int> fv{};
    auto i0 = fv.insert(10);
    auto i1 = fv.insert(20);
    auto i2 = fv.insert(30);

    EXPECT_EQ(**fv.at(i0), 10);
    EXPECT_EQ(**fv.at(i1), 20);
    EXPECT_EQ(**fv.at(i2), 30);
}

TEST(FreelistVectorTest, OperatorSubscriptReturnsReference) {
    FreelistVector<int> fv{};
    auto idx = fv.insert(99);

    EXPECT_EQ(fv[idx], 99);

    fv[idx] = 100;
    EXPECT_EQ(fv[idx], 100);
}

// ---------------------------------------------------------------------------
// At — invalid access
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, AtReturnsNulloptForOutOfRangeIndex) {
    FreelistVector<int> fv{};
    fv.insert(1);
    EXPECT_FALSE(fv.at(999).has_value());
}

TEST(FreelistVectorTest, AtReturnsNulloptForErasedSlot) {
    FreelistVector<int> fv{};
    auto idx = fv.insert(42);
    fv.erase(idx);
    EXPECT_FALSE(fv.at(idx).has_value());
}

// ---------------------------------------------------------------------------
// Erase by index
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, EraseByIndexInvalidatesSlot) {
    FreelistVector<int> fv{};
    auto i0 = fv.insert(10);
    auto i1 = fv.insert(20);
    auto i2 = fv.insert(30);

    fv.erase(i1);

    EXPECT_TRUE(fv.at(i0).has_value());
    EXPECT_FALSE(fv.at(i1).has_value());
    EXPECT_TRUE(fv.at(i2).has_value());
}

TEST(FreelistVectorTest, EraseAlreadyErasedIsNoop) {
    FreelistVector<int> fv{};
    auto idx = fv.insert(5);
    fv.erase(idx);
    fv.erase(idx); // should not crash
    EXPECT_FALSE(fv.at(idx).has_value());
}

// ---------------------------------------------------------------------------
// Freelist reuse
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, ErasedSlotIsReused) {
    FreelistVector<int> fv{};
    auto i0 = fv.insert(10);
    auto i1 = fv.insert(20);
    fv.insert(30);

    fv.erase(i0);
    fv.erase(i1);

    // Next two inserts should reclaim the freed slots
    auto r0 = fv.insert(100);
    auto r1 = fv.insert(200);

    // Reused indices come from the freelist (LIFO), so r0 == i1, r1 == i0
    EXPECT_EQ(r0, i1);
    EXPECT_EQ(r1, i0);

    EXPECT_EQ(**fv.at(r0), 100);
    EXPECT_EQ(**fv.at(r1), 200);
}

TEST(FreelistVectorTest, InsertAfterAllErasedReusesSlots) {
    FreelistVector<int> fv{};
    auto i0 = fv.insert(1);
    auto i1 = fv.insert(2);

    fv.erase(i0);
    fv.erase(i1);

    auto r0 = fv.insert(99);
    EXPECT_TRUE(fv.at(r0).has_value());
    EXPECT_EQ(**fv.at(r0), 99);
    // Should not have grown beyond original 2 entries
    EXPECT_EQ(fv.data.size(), 2u);
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, ClearRemovesEverything) {
    FreelistVector<int> fv{};
    fv.insert(1);
    fv.insert(2);
    fv.insert(3);

    fv.clear();

    EXPECT_EQ(fv.begin(), fv.end());
    EXPECT_EQ(fv.data.size(), 0u);
}

// ---------------------------------------------------------------------------
// Reserve
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, ReserveIncreasesCapacityOnly) {
    FreelistVector<int> fv{};
    fv.reserve(100);
    EXPECT_GE(fv.capacity(), 100u);
    EXPECT_EQ(fv.begin(), fv.end());
}

// ---------------------------------------------------------------------------
// Iteration — skips erased slots
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, IteratorSkipsErasedSlots) {
    FreelistVector<int> fv{};
    fv.insert(10);
    auto i1 = fv.insert(20);
    fv.insert(30);

    fv.erase(i1);

    std::vector<int> collected;
    for (auto &v : fv) {
        collected.push_back(v);
    }

    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(collected[0], 10);
    EXPECT_EQ(collected[1], 30);
}

TEST(FreelistVectorTest, IteratorOverEmptyContainer) {
    FreelistVector<int> fv{};
    int count = 0;
    for ([[maybe_unused]] auto &v : fv) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(FreelistVectorTest, IteratorOverAllErased) {
    FreelistVector<int> fv{};
    auto i0 = fv.insert(1);
    auto i1 = fv.insert(2);
    fv.erase(i0);
    fv.erase(i1);

    int count = 0;
    for ([[maybe_unused]] auto &v : fv) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

// ---------------------------------------------------------------------------
// Erase by iterator / range
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, EraseByIterator) {
    FreelistVector<int> fv{};
    auto i0 = fv.insert(10);
    fv.insert(20);

    fv.erase(fv.begin());

    EXPECT_FALSE(fv.at(i0).has_value());
}

TEST(FreelistVectorTest, EraseByIteratorRange) {
    FreelistVector<int> fv{};
    fv.insert(10);
    fv.insert(20);
    fv.insert(30);

    fv.erase(fv.begin(), fv.end());

    int count = 0;
    for ([[maybe_unused]] auto &v : fv) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

// ---------------------------------------------------------------------------
// Swap
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, SwapExchangesContents) {
    FreelistVector<int> a{};
    FreelistVector<int> b{};

    auto ai = a.insert(1);
    b.insert(100);
    b.insert(200);

    swap(a, b);

    EXPECT_EQ(a.data.size(), 2u);
    EXPECT_EQ(b.data.size(), 1u);
    EXPECT_TRUE(b.at(ai).has_value());
    EXPECT_EQ(**b.at(ai), 1);
}

// ---------------------------------------------------------------------------
// Const access
// ---------------------------------------------------------------------------

TEST(FreelistVectorTest, ConstAtAndSubscript) {
    FreelistVector<int> fv{};
    auto idx = fv.insert(77);

    const auto &cfv = fv;

    auto opt = cfv.at(idx);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(**opt, 77);

    EXPECT_EQ(cfv[idx], 77);
}

TEST(FreelistVectorTest, ConstIteration) {
    FreelistVector<int> fv{};
    fv.insert(5);
    fv.insert(10);

    const auto &cfv = fv;

    int sum = 0;
    for (const auto &v : cfv) {
        sum += v;
    }
    EXPECT_EQ(sum, 15);
}

// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
