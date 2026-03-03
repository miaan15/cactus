#include <algorithm>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

import SlotMap;

using IntSlotMap = cactus::SlotMap<int>;
using StringSlotMap = cactus::SlotMap<std::string>;

// ============================================================================
// Construction & Initial State
// ============================================================================

TEST(SlotMapConstruction, DefaultConstructedIsEmpty) {
    IntSlotMap sm{};
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
}

TEST(SlotMapConstruction, BeginEqualsEndWhenEmpty) {
    IntSlotMap sm{};
    EXPECT_EQ(sm.begin(), sm.end());
    EXPECT_EQ(sm.cbegin(), sm.cend());
    EXPECT_EQ(sm.rbegin(), sm.rend());
    EXPECT_EQ(sm.crbegin(), sm.crend());
}

// ============================================================================
// Insert / Emplace
// ============================================================================

TEST(SlotMapInsert, InsertLvalue) {
    IntSlotMap sm{};
    int val = 42;
    auto key = sm.insert(val);

    EXPECT_EQ(sm.size(), 1u);
    EXPECT_FALSE(sm.empty());
    EXPECT_EQ(sm[key], 42);
}

TEST(SlotMapInsert, InsertRvalue) {
    IntSlotMap sm{};
    auto key = sm.insert(99);

    EXPECT_EQ(sm.size(), 1u);
    EXPECT_EQ(sm[key], 99);
}

TEST(SlotMapInsert, EmplaceConstructsInPlace) {
    StringSlotMap sm{};
    auto key = sm.emplace("hello");

    EXPECT_EQ(sm.size(), 1u);
    EXPECT_EQ(sm[key], "hello");
}

TEST(SlotMapInsert, MultipleInserts) {
    IntSlotMap sm{};
    auto k0 = sm.insert(10);
    auto k1 = sm.insert(20);
    auto k2 = sm.insert(30);

    EXPECT_EQ(sm.size(), 3u);
    EXPECT_EQ(sm[k0], 10);
    EXPECT_EQ(sm[k1], 20);
    EXPECT_EQ(sm[k2], 30);
}

TEST(SlotMapInsert, KeysHaveDistinctIndices) {
    IntSlotMap sm{};
    auto k0 = sm.insert(1);
    auto k1 = sm.insert(2);
    auto k2 = sm.insert(3);

    // Each key should have a unique slot index
    EXPECT_NE(k0.index, k1.index);
    EXPECT_NE(k1.index, k2.index);
    EXPECT_NE(k0.index, k2.index);
}

// ============================================================================
// Find
// ============================================================================

TEST(SlotMapFind, FindExistingKey) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    auto it = sm.find(key);
    ASSERT_NE(it, sm.end());
    EXPECT_EQ(*it, 42);
}

TEST(SlotMapFind, FindNonExistentKeyReturnsEnd) {
    IntSlotMap sm{};
    sm.insert(42);

    IntSlotMap::key_type bad_key{999, 0};
    EXPECT_EQ(sm.find(bad_key), sm.end());
}

TEST(SlotMapFind, FindStaleKeyReturnsEnd) {
    IntSlotMap sm{};
    auto key = sm.insert(42);
    sm.erase(key);

    // key is now stale (generation mismatch)
    EXPECT_EQ(sm.find(key), sm.end());
}

TEST(SlotMapFind, ConstFind) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    const auto &csm = sm;
    auto it = csm.find(key);
    ASSERT_NE(it, csm.end());
    EXPECT_EQ(*it, 42);
}

// ============================================================================
// at()
// ============================================================================

TEST(SlotMapAt, AtReturnsReferenceForValidKey) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    EXPECT_EQ(sm.at(key), 42);
}

TEST(SlotMapAt, AtThrowsForInvalidKey) {
    IntSlotMap sm{};
    IntSlotMap::key_type bad_key{999, 0};

    EXPECT_THROW(sm.at(bad_key), std::out_of_range);
}

TEST(SlotMapAt, AtThrowsForStaleKey) {
    IntSlotMap sm{};
    auto key = sm.insert(42);
    sm.erase(key);

    EXPECT_THROW(sm.at(key), std::out_of_range);
}

TEST(SlotMapAt, AtIsMutable) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    sm.at(key) = 100;
    EXPECT_EQ(sm.at(key), 100);
}

TEST(SlotMapAt, ConstAtReturnsConstReference) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    const auto &csm = sm;
    EXPECT_EQ(csm.at(key), 42);
}

TEST(SlotMapAt, ConstAtThrowsForInvalidKey) {
    IntSlotMap sm{};
    const auto &csm = sm;

    IntSlotMap::key_type bad_key{999, 0};
    EXPECT_THROW(csm.at(bad_key), std::out_of_range);
}

// ============================================================================
// get() (optional-based access)
// ============================================================================

TEST(SlotMapGet, GetReturnsPointerForValidKey) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    auto result = sm.get(key);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 42);
}

TEST(SlotMapGet, GetReturnsNulloptForInvalidKey) {
    IntSlotMap sm{};
    IntSlotMap::key_type bad_key{999, 0};

    EXPECT_FALSE(sm.get(bad_key).has_value());
}

TEST(SlotMapGet, GetReturnsNulloptForStaleKey) {
    IntSlotMap sm{};
    auto key = sm.insert(42);
    sm.erase(key);

    EXPECT_FALSE(sm.get(key).has_value());
}

TEST(SlotMapGet, ConstGetReturnsConstPointer) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    const auto &csm = sm;
    auto result = csm.get(key);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 42);
}

// ============================================================================
// operator[]
// ============================================================================

TEST(SlotMapSubscript, ReturnsCorrectValue) {
    IntSlotMap sm{};
    auto key = sm.insert(42);
    EXPECT_EQ(sm[key], 42);
}

TEST(SlotMapSubscript, IsMutable) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    sm[key] = 100;
    EXPECT_EQ(sm[key], 100);
}

TEST(SlotMapSubscript, ConstSubscript) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    const auto &csm = sm;
    EXPECT_EQ(csm[key], 42);
}

// ============================================================================
// contains()
// ============================================================================

TEST(SlotMapContains, ReturnsTrueForValidKey) {
    IntSlotMap sm{};
    auto key = sm.insert(42);
    EXPECT_TRUE(sm.contains(key));
}

TEST(SlotMapContains, ReturnsFalseForInvalidKey) {
    IntSlotMap sm{};
    IntSlotMap::key_type bad_key{999, 0};
    EXPECT_FALSE(sm.contains(bad_key));
}

TEST(SlotMapContains, ReturnsFalseForStaleKey) {
    IntSlotMap sm{};
    auto key = sm.insert(42);
    sm.erase(key);
    EXPECT_FALSE(sm.contains(key));
}

TEST(SlotMapContains, ReturnsFalseOnEmptyMap) {
    IntSlotMap sm{};
    IntSlotMap::key_type key{0, 0};
    EXPECT_FALSE(sm.contains(key));
}

// ============================================================================
// Erase by key
// ============================================================================

TEST(SlotMapErase, EraseByKeyRemovesElement) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    sm.erase(key);
    EXPECT_EQ(sm.size(), 0u);
    EXPECT_TRUE(sm.empty());
}

TEST(SlotMapErase, EraseByKeyInvalidatesKey) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    sm.erase(key);
    EXPECT_FALSE(sm.contains(key));
    EXPECT_EQ(sm.find(key), sm.end());
}

TEST(SlotMapErase, EraseByInvalidKeyReturnsEnd) {
    IntSlotMap sm{};
    sm.insert(42);

    IntSlotMap::key_type bad_key{999, 0};
    EXPECT_EQ(sm.erase(bad_key), sm.end());
    EXPECT_EQ(sm.size(), 1u);
}

TEST(SlotMapErase, EraseMiddleElementPreservesOthers) {
    IntSlotMap sm{};
    auto k0 = sm.insert(10);
    auto k1 = sm.insert(20);
    auto k2 = sm.insert(30);

    sm.erase(k1);

    EXPECT_EQ(sm.size(), 2u);
    EXPECT_EQ(sm[k0], 10);
    EXPECT_FALSE(sm.contains(k1));
    EXPECT_EQ(sm[k2], 30);
}

TEST(SlotMapErase, EraseFirstElementPreservesOthers) {
    IntSlotMap sm{};
    auto k0 = sm.insert(10);
    auto k1 = sm.insert(20);
    auto k2 = sm.insert(30);

    sm.erase(k0);

    EXPECT_EQ(sm.size(), 2u);
    EXPECT_FALSE(sm.contains(k0));
    EXPECT_EQ(sm[k1], 20);
    EXPECT_EQ(sm[k2], 30);
}

TEST(SlotMapErase, EraseLastElementPreservesOthers) {
    IntSlotMap sm{};
    auto k0 = sm.insert(10);
    auto k1 = sm.insert(20);
    auto k2 = sm.insert(30);

    sm.erase(k2);

    EXPECT_EQ(sm.size(), 2u);
    EXPECT_EQ(sm[k0], 10);
    EXPECT_EQ(sm[k1], 20);
    EXPECT_FALSE(sm.contains(k2));
}

// ============================================================================
// Erase by iterator
// ============================================================================

TEST(SlotMapEraseIter, EraseSingleByIterator) {
    IntSlotMap sm{};
    sm.insert(42);

    auto it = sm.begin();
    sm.erase(it);
    EXPECT_TRUE(sm.empty());
}

TEST(SlotMapEraseIter, EraseReturnsValidIterator) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    auto it = sm.erase(sm.begin());
    // Returned iterator should be valid (either pointing to an element or end)
    EXPECT_EQ(sm.size(), 2u);
    if (it != sm.end()) {
        // The value at the iterator should be one of the remaining values
        EXPECT_TRUE(*it == 10 || *it == 20 || *it == 30);
    }
}

// ============================================================================
// Erase by iterator range
// ============================================================================

TEST(SlotMapEraseRange, EraseAll) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    sm.erase(sm.begin(), sm.end());
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
}

TEST(SlotMapEraseRange, EraseEmptyRange) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);

    sm.erase(sm.begin(), sm.begin());
    EXPECT_EQ(sm.size(), 2u);
}

// ============================================================================
// Slot reuse after erase
// ============================================================================

TEST(SlotMapSlotReuse, ReusesSlotAfterErase) {
    IntSlotMap sm{};
    auto k0 = sm.insert(10);
    sm.erase(k0);

    auto k1 = sm.insert(20);

    // The slot index should be reused
    EXPECT_EQ(k1.index, k0.index);
    // But the generation should have advanced
    EXPECT_NE(k1.gen, k0.gen);

    EXPECT_EQ(sm[k1], 20);
    EXPECT_FALSE(sm.contains(k0));
}

TEST(SlotMapSlotReuse, MultipleReuses) {
    IntSlotMap sm{};
    auto k0 = sm.insert(100);
    sm.erase(k0);

    auto k1 = sm.insert(200);
    sm.erase(k1);

    auto k2 = sm.insert(300);

    EXPECT_EQ(k2.index, k0.index);
    EXPECT_EQ(sm[k2], 300);
    EXPECT_FALSE(sm.contains(k0));
    EXPECT_FALSE(sm.contains(k1));
}

// ============================================================================
// Iterators
// ============================================================================

TEST(SlotMapIterators, ForwardIteration) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    std::vector<int> values(sm.begin(), sm.end());
    std::sort(values.begin(), values.end());

    EXPECT_EQ(values, (std::vector<int>{10, 20, 30}));
}

TEST(SlotMapIterators, ConstForwardIteration) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);

    const auto &csm = sm;
    std::vector<int> values(csm.begin(), csm.end());
    std::sort(values.begin(), values.end());

    EXPECT_EQ(values, (std::vector<int>{10, 20}));
}

TEST(SlotMapIterators, CBeginCEnd) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);

    std::vector<int> values(sm.cbegin(), sm.cend());
    std::sort(values.begin(), values.end());

    EXPECT_EQ(values, (std::vector<int>{10, 20}));
}

TEST(SlotMapIterators, ReverseIteration) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    std::vector<int> values(sm.rbegin(), sm.rend());
    // Reverse of the data order
    std::vector<int> forward_values(sm.begin(), sm.end());
    std::reverse(forward_values.begin(), forward_values.end());

    EXPECT_EQ(values, forward_values);
}

TEST(SlotMapIterators, ConstReverseIteration) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);

    const auto &csm = sm;
    std::vector<int> values(csm.rbegin(), csm.rend());
    EXPECT_EQ(values.size(), 2u);
}

TEST(SlotMapIterators, CRBeginCREnd) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);

    std::vector<int> values(sm.crbegin(), sm.crend());
    EXPECT_EQ(values.size(), 2u);
}

TEST(SlotMapIterators, RangeBasedForLoop) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    int sum = 0;
    for (auto val : sm) {
        sum += val;
    }
    EXPECT_EQ(sum, 60);
}

TEST(SlotMapIterators, MutateViaIterator) {
    IntSlotMap sm{};
    auto key = sm.insert(42);

    *sm.begin() = 100;
    // Since there's only one element, sm[key] must reflect the change
    EXPECT_EQ(sm[key], 100);
}

// ============================================================================
// clear()
// ============================================================================

TEST(SlotMapClear, ClearEmptiesTheMap) {
    IntSlotMap sm{};
    auto k0 = sm.insert(10);
    auto k1 = sm.insert(20);
    auto k2 = sm.insert(30);

    sm.clear();

    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
    EXPECT_EQ(sm.begin(), sm.end());

    // Old keys should not be found
    EXPECT_EQ(sm.find(k0), sm.end());
    EXPECT_EQ(sm.find(k1), sm.end());
    EXPECT_EQ(sm.find(k2), sm.end());
}

TEST(SlotMapClear, InsertAfterClearWorks) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.clear();

    auto key = sm.insert(99);
    EXPECT_EQ(sm.size(), 1u);
    EXPECT_EQ(sm[key], 99);
}

// ============================================================================
// reserve()
// ============================================================================

TEST(SlotMapReserve, ReserveIncreasesCapacity) {
    IntSlotMap sm{};
    sm.reserve(100);

    EXPECT_GE(sm.capacity(), 100u);
    EXPECT_TRUE(sm.empty());
}

TEST(SlotMapReserve, ReserveDoesNotChangeSize) {
    IntSlotMap sm{};
    sm.insert(10);
    sm.reserve(100);

    EXPECT_EQ(sm.size(), 1u);
}

// ============================================================================
// Size / Empty / Capacity
// ============================================================================

TEST(SlotMapSizeEmpty, EmptyOnConstruction) {
    IntSlotMap sm{};
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
}

TEST(SlotMapSizeEmpty, NotEmptyAfterInsert) {
    IntSlotMap sm{};
    sm.insert(42);
    EXPECT_FALSE(sm.empty());
    EXPECT_EQ(sm.size(), 1u);
}

TEST(SlotMapSizeEmpty, SizeTracksInsertAndErase) {
    IntSlotMap sm{};
    auto k0 = sm.insert(10);
    auto k1 = sm.insert(20);
    EXPECT_EQ(sm.size(), 2u);

    sm.erase(k0);
    EXPECT_EQ(sm.size(), 1u);

    sm.erase(k1);
    EXPECT_EQ(sm.size(), 0u);
    EXPECT_TRUE(sm.empty());
}

TEST(SlotMapSizeEmpty, CapacityIsAtLeastSize) {
    IntSlotMap sm{};
    for (int i = 0; i < 50; ++i) {
        sm.insert(i);
    }
    EXPECT_GE(sm.capacity(), sm.size());
}

// ============================================================================
// Stress / Complex Scenarios
// ============================================================================

TEST(SlotMapStress, InsertEraseInsertCycle) {
    IntSlotMap sm{};
    std::vector<IntSlotMap::key_type> keys;

    // Insert 100 elements
    for (int i = 0; i < 100; ++i) {
        keys.push_back(sm.insert(i));
    }
    EXPECT_EQ(sm.size(), 100u);

    // Erase every other element
    for (int i = 0; i < 100; i += 2) {
        sm.erase(keys[i]);
    }
    EXPECT_EQ(sm.size(), 50u);

    // Verify remaining elements are accessible
    for (int i = 1; i < 100; i += 2) {
        EXPECT_TRUE(sm.contains(keys[i]));
        EXPECT_EQ(sm[keys[i]], i);
    }

    // Verify erased elements are gone
    for (int i = 0; i < 100; i += 2) {
        EXPECT_FALSE(sm.contains(keys[i]));
    }

    // Insert 50 more elements (should reuse slots)
    for (int i = 100; i < 150; ++i) {
        auto key = sm.insert(i);
        EXPECT_TRUE(sm.contains(key));
        EXPECT_EQ(sm[key], i);
    }
    EXPECT_EQ(sm.size(), 100u);
}

TEST(SlotMapStress, EraseAllThenRefill) {
    IntSlotMap sm{};
    std::vector<IntSlotMap::key_type> keys;

    for (int i = 0; i < 50; ++i) {
        keys.push_back(sm.insert(i));
    }

    // Erase all
    for (auto &key : keys) {
        sm.erase(key);
    }
    EXPECT_TRUE(sm.empty());

    // None of the old keys should work
    for (auto &key : keys) {
        EXPECT_FALSE(sm.contains(key));
    }

    // Re-insert
    std::vector<IntSlotMap::key_type> new_keys;
    for (int i = 0; i < 50; ++i) {
        new_keys.push_back(sm.insert(i + 1000));
    }
    EXPECT_EQ(sm.size(), 50u);

    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(sm[new_keys[i]], i + 1000);
    }
}

TEST(SlotMapStress, DataContiguityAfterErases) {
    IntSlotMap sm{};
    auto k0 = sm.insert(10);
    auto k1 = sm.insert(20);
    auto k2 = sm.insert(30);
    auto k3 = sm.insert(40);
    auto k4 = sm.insert(50);

    sm.erase(k1);
    sm.erase(k3);

    // Data should still be contiguous and iterable
    EXPECT_EQ(sm.size(), 3u);

    std::vector<int> values(sm.begin(), sm.end());
    std::sort(values.begin(), values.end());
    EXPECT_EQ(values, (std::vector<int>{10, 30, 50}));

    // Remaining keys still valid
    EXPECT_EQ(sm[k0], 10);
    EXPECT_EQ(sm[k2], 30);
    EXPECT_EQ(sm[k4], 50);
}

// ============================================================================
// String type (non-trivial value_type)
// ============================================================================

TEST(SlotMapString, InsertAndRetrieve) {
    StringSlotMap sm{};
    auto key = sm.emplace("hello world");

    EXPECT_EQ(sm[key], "hello world");
    EXPECT_EQ(sm.at(key), "hello world");
}

TEST(SlotMapString, EraseAndReinnsert) {
    StringSlotMap sm{};
    auto k0 = sm.emplace("alpha");
    auto k1 = sm.emplace("beta");

    sm.erase(k0);
    EXPECT_FALSE(sm.contains(k0));
    EXPECT_EQ(sm[k1], "beta");

    auto k2 = sm.emplace("gamma");
    EXPECT_EQ(sm[k2], "gamma");
    EXPECT_EQ(sm.size(), 2u);
}

TEST(SlotMapString, MutateViaAt) {
    StringSlotMap sm{};
    auto key = sm.emplace("before");

    sm.at(key) = "after";
    EXPECT_EQ(sm[key], "after");
}

// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
