#include <algorithm>
#include <gtest/gtest.h>
#include <numeric>
#include <string>
#include <tuplet/tuple.hpp>
#include <vector>

import SlotMap;

using namespace cactus;

// ============================================================================
// Test Fixture
// ============================================================================

class SlotMapTest : public ::testing::Test {
protected:
    SlotMap<int> sm;
    SlotMap<std::string> ssm;
};

// ============================================================================
// Construction & Initial State
// ============================================================================

TEST_F(SlotMapTest, DefaultConstructedIsEmpty) {
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
}

TEST_F(SlotMapTest, DefaultConstructedBeginEqualsEnd) {
    EXPECT_EQ(sm.begin(), sm.end());
    EXPECT_EQ(sm.cbegin(), sm.cend());
    EXPECT_EQ(sm.rbegin(), sm.rend());
    EXPECT_EQ(sm.crbegin(), sm.crend());
}

// ============================================================================
// Insert / Emplace
// ============================================================================

TEST_F(SlotMapTest, InsertSingleElement) {
    auto key = sm.insert(42);
    EXPECT_EQ(sm.size(), 1u);
    EXPECT_FALSE(sm.empty());
    EXPECT_EQ(sm[key], 42);
}

TEST_F(SlotMapTest, InsertMultipleElements) {
    auto k1 = sm.insert(10);
    auto k2 = sm.insert(20);
    auto k3 = sm.insert(30);

    EXPECT_EQ(sm.size(), 3u);
    EXPECT_EQ(sm[k1], 10);
    EXPECT_EQ(sm[k2], 20);
    EXPECT_EQ(sm[k3], 30);
}

TEST_F(SlotMapTest, InsertRvalue) {
    std::string val = "hello";
    auto key = ssm.insert(std::move(val));
    EXPECT_EQ(ssm[key], "hello");
}

TEST_F(SlotMapTest, InsertLvalue) {
    const std::string val = "world";
    auto key = ssm.insert(val);
    EXPECT_EQ(ssm[key], "world");
    EXPECT_EQ(val, "world"); // original unchanged
}

TEST_F(SlotMapTest, EmplaceConstructsInPlace) {
    auto key = ssm.emplace("constructed");
    EXPECT_EQ(ssm[key], "constructed");
    EXPECT_EQ(ssm.size(), 1u);
}

TEST_F(SlotMapTest, EmplaceMultiple) {
    std::vector<SlotMap<int>::key_type> keys;
    for (int i = 0; i < 100; ++i) {
        keys.push_back(sm.emplace(i));
    }
    EXPECT_EQ(sm.size(), 100u);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(sm[keys[i]], i);
    }
}

// ============================================================================
// Key Uniqueness & Validity
// ============================================================================

TEST_F(SlotMapTest, InsertedKeysAreUnique) {
    auto k1 = sm.insert(1);
    auto k2 = sm.insert(2);
    auto k3 = sm.insert(3);

    EXPECT_NE(k1, k2);
    EXPECT_NE(k1, k3);
    EXPECT_NE(k2, k3);
}

TEST_F(SlotMapTest, KeyEqualityAndInequality) {
    auto k1 = sm.insert(1);
    auto k2 = sm.insert(2);

    SlotMap<int>::key_type k1_copy = k1;
    EXPECT_EQ(k1, k1_copy);
    EXPECT_NE(k1, k2);
}

// ============================================================================
// Find
// ============================================================================

TEST_F(SlotMapTest, FindValidKey) {
    auto key = sm.insert(42);
    auto it = sm.find(key);
    ASSERT_NE(it, sm.end());
    EXPECT_EQ(*it, 42);
}

TEST_F(SlotMapTest, FindInvalidKeyOutOfRange) {
    sm.insert(42);
    SlotMap<int>::key_type bad_key{999, 0};
    EXPECT_EQ(sm.find(bad_key), sm.end());
}

TEST_F(SlotMapTest, FindStaleKeyAfterErase) {
    auto key = sm.insert(42);
    sm.erase(sm.find(key));
    EXPECT_EQ(sm.find(key), sm.end());
}

TEST_F(SlotMapTest, FindWithTupleKey) {
    auto key = sm.insert(42);
    tuplet::tuple<size_t, size_t> tuple_key = key;
    auto it = sm.find(tuple_key);
    ASSERT_NE(it, sm.end());
    EXPECT_EQ(*it, 42);
}

TEST_F(SlotMapTest, FindConstOverload) {
    auto key = sm.insert(42);
    const auto &csm = sm;
    auto it = csm.find(key);
    ASSERT_NE(it, csm.end());
    EXPECT_EQ(*it, 42);
}

TEST_F(SlotMapTest, FindConstWithTupleKey) {
    auto key = sm.insert(42);
    const auto &csm = sm;
    tuplet::tuple<size_t, size_t> tuple_key = key;
    auto it = csm.find(tuple_key);
    ASSERT_NE(it, csm.end());
    EXPECT_EQ(*it, 42);
}

// ============================================================================
// At
// ============================================================================

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

TEST_F(SlotMapTest, AtValidKey) {
    auto key = sm.insert(42);
    EXPECT_EQ(sm.at(key), 42);
}

TEST_F(SlotMapTest, AtInvalidKeyThrows) {
    SlotMap<int>::key_type bad_key{0, 0};
    EXPECT_THROW(sm.at(bad_key), std::out_of_range);
}

TEST_F(SlotMapTest, AtStaleKeyThrows) {
    auto key = sm.insert(42);
    sm.erase(sm.find(key));
    EXPECT_THROW(sm.at(key), std::out_of_range);
}

TEST_F(SlotMapTest, AtWithTupleKey) {
    auto key = sm.insert(42);
    tuplet::tuple<size_t, size_t> tuple_key = key;
    EXPECT_EQ(sm.at(tuple_key), 42);
}

TEST_F(SlotMapTest, AtConstOverload) {
    auto key = sm.insert(42);
    const auto &csm = sm;
    EXPECT_EQ(csm.at(key), 42);
}

TEST_F(SlotMapTest, AtConstInvalidKeyThrows) {
    const auto &csm = sm;
    SlotMap<int>::key_type bad_key{0, 0};
    EXPECT_THROW(csm.at(bad_key), std::out_of_range);
}

TEST_F(SlotMapTest, AtConstWithTupleKey) {
    auto key = sm.insert(99);
    const auto &csm = sm;
    tuplet::tuple<size_t, size_t> tuple_key = key;
    EXPECT_EQ(csm.at(tuple_key), 99);
}

TEST_F(SlotMapTest, AtReturnsMutableReference) {
    auto key = sm.insert(42);
    sm.at(key) = 100;
    EXPECT_EQ(sm.at(key), 100);
}

#pragma GCC diagnostic pop

// ============================================================================
// operator[]
// ============================================================================

TEST_F(SlotMapTest, SubscriptOperatorAccess) {
    auto key = sm.insert(42);
    EXPECT_EQ(sm[key], 42);
}

TEST_F(SlotMapTest, SubscriptOperatorMutation) {
    auto key = sm.insert(42);
    sm[key] = 100;
    EXPECT_EQ(sm[key], 100);
}

TEST_F(SlotMapTest, SubscriptOperatorWithTupleKey) {
    auto key = sm.insert(42);
    tuplet::tuple<size_t, size_t> tuple_key = key;
    EXPECT_EQ(sm[tuple_key], 42);
}

TEST_F(SlotMapTest, SubscriptOperatorConst) {
    auto key = sm.insert(42);
    const auto &csm = sm;
    EXPECT_EQ(csm[key], 42);
}

TEST_F(SlotMapTest, SubscriptOperatorConstWithTupleKey) {
    auto key = sm.insert(42);
    const auto &csm = sm;
    tuplet::tuple<size_t, size_t> tuple_key = key;
    EXPECT_EQ(csm[tuple_key], 42);
}

// ============================================================================
// Contains
// ============================================================================

TEST_F(SlotMapTest, ContainsValidKey) {
    auto key = sm.insert(42);
    EXPECT_TRUE(sm.contains(key));
}

TEST_F(SlotMapTest, ContainsInvalidKey) {
    SlotMap<int>::key_type bad_key{999, 0};
    EXPECT_FALSE(sm.contains(bad_key));
}

TEST_F(SlotMapTest, ContainsStaleKey) {
    auto key = sm.insert(42);
    sm.erase(sm.find(key));
    EXPECT_FALSE(sm.contains(key));
}

TEST_F(SlotMapTest, ContainsWithTupleKey) {
    auto key = sm.insert(42);
    tuplet::tuple<size_t, size_t> tuple_key = key;
    EXPECT_TRUE(sm.contains(tuple_key));
}

TEST_F(SlotMapTest, ContainsEmptyMap) {
    SlotMap<int>::key_type key{0, 0};
    EXPECT_FALSE(sm.contains(key));
}

// ============================================================================
// Erase
// ============================================================================

TEST_F(SlotMapTest, EraseSingleElement) {
    auto key = sm.insert(42);
    sm.erase(sm.find(key));
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
}

TEST_F(SlotMapTest, EraseByKey) {
    auto key = sm.insert(42);
    auto it = sm.erase(key);
    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(it, sm.end());
}

TEST_F(SlotMapTest, EraseByInvalidKeyReturnsEnd) {
    sm.insert(42);
    SlotMap<int>::key_type bad_key{999, 0};
    auto it = sm.erase(bad_key);
    EXPECT_EQ(it, sm.end());
    EXPECT_EQ(sm.size(), 1u);
}

TEST_F(SlotMapTest, EraseFirstOfThree) {
    auto k1 = sm.insert(10);
    auto k2 = sm.insert(20);
    auto k3 = sm.insert(30);

    sm.erase(sm.find(k1));
    EXPECT_EQ(sm.size(), 2u);
    EXPECT_FALSE(sm.contains(k1));
    EXPECT_TRUE(sm.contains(k2));
    EXPECT_TRUE(sm.contains(k3));
    EXPECT_EQ(sm[k2], 20);
    EXPECT_EQ(sm[k3], 30);
}

TEST_F(SlotMapTest, EraseMiddleOfThree) {
    auto k1 = sm.insert(10);
    auto k2 = sm.insert(20);
    auto k3 = sm.insert(30);

    sm.erase(sm.find(k2));
    EXPECT_EQ(sm.size(), 2u);
    EXPECT_TRUE(sm.contains(k1));
    EXPECT_FALSE(sm.contains(k2));
    EXPECT_TRUE(sm.contains(k3));
    EXPECT_EQ(sm[k1], 10);
    EXPECT_EQ(sm[k3], 30);
}

TEST_F(SlotMapTest, EraseLastOfThree) {
    auto k1 = sm.insert(10);
    auto k2 = sm.insert(20);
    auto k3 = sm.insert(30);

    sm.erase(sm.find(k3));
    EXPECT_EQ(sm.size(), 2u);
    EXPECT_TRUE(sm.contains(k1));
    EXPECT_TRUE(sm.contains(k2));
    EXPECT_FALSE(sm.contains(k3));
    EXPECT_EQ(sm[k1], 10);
    EXPECT_EQ(sm[k2], 20);
}

TEST_F(SlotMapTest, EraseAllOneByOne) {
    auto k1 = sm.insert(10);
    auto k2 = sm.insert(20);
    auto k3 = sm.insert(30);

    sm.erase(sm.find(k1));
    sm.erase(sm.find(k2));
    sm.erase(sm.find(k3));

    EXPECT_TRUE(sm.empty());
}

TEST_F(SlotMapTest, EraseRange) {
    for (int i = 0; i < 5; ++i) sm.insert(i * 10);
    EXPECT_EQ(sm.size(), 5u);

    auto first = std::next(sm.cbegin(), 1);
    auto last = std::next(sm.cbegin(), 3);
    sm.erase(first, last);

    EXPECT_EQ(sm.size(), 3u);
}

TEST_F(SlotMapTest, EraseEmptyRange) {
    auto key = sm.insert(42);
    auto it = sm.erase(sm.cbegin(), sm.cbegin());
    EXPECT_EQ(sm.size(), 1u);
    EXPECT_EQ(sm[key], 42);
    EXPECT_EQ(it, sm.begin());
}

TEST_F(SlotMapTest, EraseFullRange) {
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);
    sm.erase(sm.cbegin(), sm.cend());
    EXPECT_TRUE(sm.empty());
}

TEST_F(SlotMapTest, EraseWithMutableIterator) {
    auto key = sm.insert(42);
    sm.erase(sm.begin());
    EXPECT_TRUE(sm.empty());
}

TEST_F(SlotMapTest, EraseRangeWithMutableIterators) {
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);
    sm.erase(sm.begin(), sm.end());
    EXPECT_TRUE(sm.empty());
}

TEST_F(SlotMapTest, EraseReturnsValidIterator) {
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    auto it = sm.erase(sm.begin());
    // After erasing the first element, the returned iterator should be valid
    EXPECT_EQ(sm.size(), 2u);
    if (it != sm.end()) {
        // Just ensure we can dereference it without crashing
        (void)*it;
    }
}

// ============================================================================
// Generational Safety (Slot Reuse)
// ============================================================================

TEST_F(SlotMapTest, SlotReuseInvalidatesOldKey) {
    auto old_key = sm.insert(42);
    sm.erase(sm.find(old_key));

    auto new_key = sm.insert(99);
    // Old key should no longer be valid
    EXPECT_FALSE(sm.contains(old_key));
    // New key should be valid
    EXPECT_TRUE(sm.contains(new_key));
    EXPECT_EQ(sm[new_key], 99);
}

TEST_F(SlotMapTest, GenerationIncrements) {
    auto k1 = sm.insert(42);
    auto gen1 = k1.gen;
    sm.erase(sm.find(k1));

    auto k2 = sm.insert(99);
    // If the slot is reused, the generation should have incremented
    if (k1.index == k2.index) {
        EXPECT_GT(k2.gen, gen1);
    }
}

TEST_F(SlotMapTest, MultipleInsertEraseReuseCycles) {
    std::vector<SlotMap<int>::key_type> stale_keys;

    for (int cycle = 0; cycle < 10; ++cycle) {
        auto key = sm.insert(cycle);
        stale_keys.push_back(key);
        sm.erase(sm.find(key));
    }

    auto final_key = sm.insert(999);
    EXPECT_TRUE(sm.contains(final_key));
    EXPECT_EQ(sm[final_key], 999);

    for (const auto &stale : stale_keys) {
        EXPECT_FALSE(sm.contains(stale));
    }
}

// ============================================================================
// Insert After Erase (Freelist Behavior)
// ============================================================================

TEST_F(SlotMapTest, InsertAfterEraseReuseSlot) {
    auto k1 = sm.insert(10);
    auto k2 = sm.insert(20);
    sm.erase(sm.find(k1));

    auto k3 = sm.insert(30);
    EXPECT_EQ(sm.size(), 2u);
    EXPECT_TRUE(sm.contains(k2));
    EXPECT_TRUE(sm.contains(k3));
    EXPECT_EQ(sm[k2], 20);
    EXPECT_EQ(sm[k3], 30);
}

TEST_F(SlotMapTest, InsertAfterMultipleErases) {
    auto k1 = sm.insert(10);
    auto k2 = sm.insert(20);
    auto k3 = sm.insert(30);

    sm.erase(sm.find(k2));
    sm.erase(sm.find(k1));

    auto k4 = sm.insert(40);
    auto k5 = sm.insert(50);

    EXPECT_EQ(sm.size(), 3u);
    EXPECT_TRUE(sm.contains(k3));
    EXPECT_TRUE(sm.contains(k4));
    EXPECT_TRUE(sm.contains(k5));
    EXPECT_EQ(sm[k3], 30);
    EXPECT_EQ(sm[k4], 40);
    EXPECT_EQ(sm[k5], 50);
}

// ============================================================================
// Clear
// ============================================================================

TEST_F(SlotMapTest, ClearEmptiesMap) {
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);
    sm.clear();

    EXPECT_TRUE(sm.empty());
    EXPECT_EQ(sm.size(), 0u);
    EXPECT_EQ(sm.begin(), sm.end());
}

TEST_F(SlotMapTest, ClearThenInsert) {
    auto old_key = sm.insert(42);
    sm.clear();
    auto new_key = sm.insert(99);

    EXPECT_EQ(sm.size(), 1u);
    EXPECT_EQ(sm[new_key], 99);
    // Old key should not be valid after clear
    EXPECT_FALSE(sm.contains(old_key));
}

TEST_F(SlotMapTest, ClearOnEmpty) {
    sm.clear();
    EXPECT_TRUE(sm.empty());
}

// ============================================================================
// Reserve
// ============================================================================

TEST_F(SlotMapTest, ReserveIncreasesCapacity) {
    sm.reserve(100);
    EXPECT_GE(sm.capacity(), 100u);
    EXPECT_TRUE(sm.empty());
}

TEST_F(SlotMapTest, ReserveDoesNotChangeSize) {
    sm.insert(1);
    sm.insert(2);
    sm.reserve(100);
    EXPECT_EQ(sm.size(), 2u);
}

// ============================================================================
// Iterators
// ============================================================================

TEST_F(SlotMapTest, IteratorTraversal) {
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    int sum = 0;
    for (auto it = sm.begin(); it != sm.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 60);
}

TEST_F(SlotMapTest, ConstIteratorTraversal) {
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    const auto &csm = sm;
    int sum = 0;
    for (auto it = csm.begin(); it != csm.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 60);
}

TEST_F(SlotMapTest, CIterators) {
    sm.insert(1);
    sm.insert(2);

    int sum = 0;
    for (auto it = sm.cbegin(); it != sm.cend(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 3);
}

TEST_F(SlotMapTest, RangeBasedFor) {
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    int sum = 0;
    for (auto val : sm) {
        sum += val;
    }
    EXPECT_EQ(sum, 60);
}

TEST_F(SlotMapTest, ReverseIteratorTraversal) {
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    std::vector<int> reversed;
    for (auto it = sm.rbegin(); it != sm.rend(); ++it) {
        reversed.push_back(*it);
    }
    ASSERT_EQ(reversed.size(), 3u);
    // Reverse of data order
    EXPECT_EQ(reversed[0], 30);
    EXPECT_EQ(reversed[1], 20);
    EXPECT_EQ(reversed[2], 10);
}

TEST_F(SlotMapTest, ConstReverseIterators) {
    sm.insert(1);
    sm.insert(2);

    const auto &csm = sm;
    int sum = 0;
    for (auto it = csm.rbegin(); it != csm.rend(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 3);
}

TEST_F(SlotMapTest, CReverseIterators) {
    sm.insert(5);
    sm.insert(10);

    int sum = 0;
    for (auto it = sm.crbegin(); it != sm.crend(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 15);
}

TEST_F(SlotMapTest, IteratorMutation) {
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);

    for (auto &val : sm) {
        val *= 10;
    }

    int sum = 0;
    for (auto val : sm) {
        sum += val;
    }
    EXPECT_EQ(sum, 60);
}

// ============================================================================
// Size / Empty / Capacity
// ============================================================================

TEST_F(SlotMapTest, SizeGrowsOnInsert) {
    EXPECT_EQ(sm.size(), 0u);
    sm.insert(1);
    EXPECT_EQ(sm.size(), 1u);
    sm.insert(2);
    EXPECT_EQ(sm.size(), 2u);
}

TEST_F(SlotMapTest, SizeShrinksOnErase) {
    auto k1 = sm.insert(1);
    sm.insert(2);
    EXPECT_EQ(sm.size(), 2u);
    sm.erase(sm.find(k1));
    EXPECT_EQ(sm.size(), 1u);
}

TEST_F(SlotMapTest, EmptyAfterEraseAll) {
    auto k1 = sm.insert(1);
    sm.erase(sm.find(k1));
    EXPECT_TRUE(sm.empty());
}

// ============================================================================
// Swap
// ============================================================================

TEST_F(SlotMapTest, MemberSwap) {
    SlotMap<int> a, b;
    auto ka = a.insert(10);
    auto kb1 = b.insert(20);
    auto kb2 = b.insert(30);

    a.swap(b);

    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(b.size(), 1u);
    EXPECT_TRUE(a.contains(kb1));
    EXPECT_TRUE(a.contains(kb2));
    EXPECT_TRUE(b.contains(ka));
    EXPECT_EQ(a[kb1], 20);
    EXPECT_EQ(a[kb2], 30);
    EXPECT_EQ(b[ka], 10);
}

TEST_F(SlotMapTest, FriendSwap) {
    SlotMap<int> a, b;
    auto ka = a.insert(10);
    auto kb = b.insert(20);

    swap(a, b);

    EXPECT_EQ(a.size(), 1u);
    EXPECT_EQ(b.size(), 1u);
    EXPECT_EQ(a[kb], 20);
    EXPECT_EQ(b[ka], 10);
}

TEST_F(SlotMapTest, SwapWithEmpty) {
    SlotMap<int> a, b;
    auto ka = a.insert(42);

    a.swap(b);

    EXPECT_TRUE(a.empty());
    EXPECT_EQ(b.size(), 1u);
    EXPECT_EQ(b[ka], 42);
}

TEST_F(SlotMapTest, SwapBothEmpty) {
    SlotMap<int> a, b;
    a.swap(b);
    EXPECT_TRUE(a.empty());
    EXPECT_TRUE(b.empty());
}

// ============================================================================
// key_type Conversions
// ============================================================================

TEST_F(SlotMapTest, KeyToTupleConversion) {
    auto key = sm.insert(42);
    tuplet::tuple<size_t, size_t> t = key;
    EXPECT_EQ(tuplet::get<0>(t), key.index);
    EXPECT_EQ(tuplet::get<1>(t), key.gen);
}

TEST_F(SlotMapTest, TupleToKeyConversion) {
    auto key = sm.insert(42);
    tuplet::tuple<size_t, size_t> t = key;
    auto reconstructed = SlotMap<int>::key_type::from(t);
    EXPECT_EQ(reconstructed, key);
}

TEST_F(SlotMapTest, TupleRoundTrip) {
    auto key = sm.insert(42);
    tuplet::tuple<size_t, size_t> t = key;
    auto key2 = SlotMap<int>::key_type::from(t);
    EXPECT_EQ(sm[key2], 42);
    EXPECT_TRUE(sm.contains(key2));
}

// ============================================================================
// Data Integrity Under Stress
// ============================================================================

TEST_F(SlotMapTest, LargeInsertAndVerify) {
    constexpr int N = 1000;
    std::vector<SlotMap<int>::key_type> keys;
    keys.reserve(N);

    for (int i = 0; i < N; ++i) {
        keys.push_back(sm.insert(i));
    }

    EXPECT_EQ(sm.size(), static_cast<size_t>(N));
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(sm[keys[i]], i);
    }
}

TEST_F(SlotMapTest, AlternatingInsertErase) {
    std::vector<SlotMap<int>::key_type> active_keys;

    for (int i = 0; i < 100; ++i) {
        auto key = sm.insert(i);
        active_keys.push_back(key);

        if (i % 3 == 0 && !active_keys.empty()) {
            auto victim = active_keys.front();
            active_keys.erase(active_keys.begin());
            sm.erase(sm.find(victim));
        }
    }

    for (const auto &key : active_keys) {
        EXPECT_TRUE(sm.contains(key));
    }
    EXPECT_EQ(sm.size(), active_keys.size());
}

TEST_F(SlotMapTest, EraseAllThenRefill) {
    constexpr int N = 50;
    std::vector<SlotMap<int>::key_type> keys;

    for (int i = 0; i < N; ++i) {
        keys.push_back(sm.insert(i));
    }

    for (auto &key : keys) {
        sm.erase(sm.find(key));
    }
    EXPECT_TRUE(sm.empty());

    // All old keys should be invalid
    for (const auto &key : keys) {
        EXPECT_FALSE(sm.contains(key));
    }

    // Refill
    std::vector<SlotMap<int>::key_type> new_keys;
    for (int i = 0; i < N; ++i) {
        new_keys.push_back(sm.insert(i + 1000));
    }
    EXPECT_EQ(sm.size(), static_cast<size_t>(N));

    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(sm[new_keys[i]], i + 1000);
    }
}

TEST_F(SlotMapTest, DataContiguityAfterErase) {
    // Slot maps guarantee contiguous data storage.
    // After erasing, data should still be dense.
    auto k1 = sm.insert(10);
    sm.insert(20);
    sm.insert(30);
    sm.insert(40);

    sm.erase(sm.find(k1));

    // Data should be contiguous with size 3
    EXPECT_EQ(sm.size(), 3u);
    EXPECT_EQ(static_cast<size_t>(std::distance(sm.begin(), sm.end())), sm.size());

    int sum = 0;
    for (auto val : sm) {
        sum += val;
    }
    // 20 + 30 + 40 = 90
    EXPECT_EQ(sum, 90);
}

// ============================================================================
// Complex Value Types
// ============================================================================

TEST_F(SlotMapTest, StringValues) {
    auto k1 = ssm.insert("hello");
    auto k2 = ssm.insert("world");

    EXPECT_EQ(ssm[k1], "hello");
    EXPECT_EQ(ssm[k2], "world");

    ssm.erase(ssm.find(k1));
    EXPECT_EQ(ssm.size(), 1u);
    EXPECT_EQ(ssm[k2], "world");
}

struct MoveOnly {
    int value;
    explicit MoveOnly(int v) : value(v) {}
    MoveOnly(MoveOnly &&) = default;
    MoveOnly &operator=(MoveOnly &&) = default;
    MoveOnly(const MoveOnly &) = delete;
    MoveOnly &operator=(const MoveOnly &) = delete;
};

TEST(SlotMapMoveOnlyTest, EmplaceMoveOnlyType) {
    SlotMap<MoveOnly> sm;
    auto key = sm.emplace(42);
    EXPECT_EQ(sm[key].value, 42);
}

TEST(SlotMapMoveOnlyTest, EraseMoveOnlyType) {
    SlotMap<MoveOnly> sm;
    auto k1 = sm.emplace(10);
    auto k2 = sm.emplace(20);
    auto k3 = sm.emplace(30);

    sm.erase(sm.find(k1));
    EXPECT_EQ(sm.size(), 2u);
    EXPECT_EQ(sm[k2].value, 20);
    EXPECT_EQ(sm[k3].value, 30);
}

struct Counter {
    static int instances;
    int id;
    Counter(int i) : id(i) {
        ++instances;
    }
    Counter(const Counter &o) : id(o.id) {
        ++instances;
    }
    Counter(Counter &&o) : id(o.id) {
        ++instances;
        o.id = -1;
    }
    Counter &operator=(const Counter &) = default;
    Counter &operator=(Counter &&) = default;
    ~Counter() {
        --instances;
    }
};
int Counter::instances = 0;

TEST(SlotMapLifetimeTest, DestructorsCalled) {
    Counter::instances = 0;
    {
        SlotMap<Counter> sm;
        sm.emplace(1);
        sm.emplace(2);
        sm.emplace(3);
        EXPECT_EQ(sm.size(), 3u);
    }
    EXPECT_EQ(Counter::instances, 0);
}

TEST(SlotMapLifetimeTest, DestructorsCalledOnErase) {
    Counter::instances = 0;
    SlotMap<Counter> sm;
    auto k1 = sm.emplace(1);
    sm.emplace(2);
    int before = Counter::instances;
    sm.erase(sm.find(k1));
    // One fewer logical element, though move semantics may temporarily affect count
    EXPECT_EQ(sm.size(), 1u);
    // After erase completes, pop_back destroys the moved-from tail element
    EXPECT_LT(Counter::instances, before);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(SlotMapTest, SingleInsertEraseCycle) {
    for (int i = 0; i < 100; ++i) {
        auto key = sm.insert(i);
        EXPECT_EQ(sm.size(), 1u);
        EXPECT_EQ(sm[key], i);
        sm.erase(sm.find(key));
        EXPECT_TRUE(sm.empty());
    }
}

TEST_F(SlotMapTest, EraseReverseOrder) {
    std::vector<SlotMap<int>::key_type> keys;
    for (int i = 0; i < 10; ++i) {
        keys.push_back(sm.insert(i));
    }

    for (int i = 9; i >= 0; --i) {
        sm.erase(sm.find(keys[i]));
        EXPECT_EQ(sm.size(), static_cast<size_t>(i));
    }
    EXPECT_TRUE(sm.empty());
}

TEST_F(SlotMapTest, EraseForwardOrder) {
    std::vector<SlotMap<int>::key_type> keys;
    for (int i = 0; i < 10; ++i) {
        keys.push_back(sm.insert(i));
    }

    for (int i = 0; i < 10; ++i) {
        sm.erase(sm.find(keys[i]));
    }
    EXPECT_TRUE(sm.empty());
}

TEST_F(SlotMapTest, FindOnEmptyMap) {
    SlotMap<int>::key_type key{0, 0};
    EXPECT_EQ(sm.find(key), sm.end());
}

TEST_F(SlotMapTest, StdAlgorithmsWork) {
    sm.insert(30);
    sm.insert(10);
    sm.insert(20);

    std::sort(sm.begin(), sm.end());
    EXPECT_EQ(*sm.begin(), 10);
    EXPECT_EQ(*std::next(sm.begin()), 20);
    EXPECT_EQ(*std::next(sm.begin(), 2), 30);
}

TEST_F(SlotMapTest, StdFindOnData) {
    sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    auto it = std::find(sm.begin(), sm.end(), 20);
    ASSERT_NE(it, sm.end());
    EXPECT_EQ(*it, 20);

    it = std::find(sm.begin(), sm.end(), 999);
    EXPECT_EQ(it, sm.end());
}

TEST_F(SlotMapTest, StdAccumulate) {
    sm.insert(1);
    sm.insert(2);
    sm.insert(3);
    sm.insert(4);

    int sum = std::accumulate(sm.begin(), sm.end(), 0);
    EXPECT_EQ(sum, 10);
}

// ============================================================================
// Custom Generation Type
// ============================================================================

TEST(SlotMapCustomGenTest, UnsignedIntGeneration) {
    SlotMap<int, unsigned int> sm;
    auto k1 = sm.insert(42);
    EXPECT_EQ(sm[k1], 42);

    sm.erase(sm.find(k1));
    EXPECT_FALSE(sm.contains(k1));

    auto k2 = sm.insert(99);
    EXPECT_TRUE(sm.contains(k2));
    EXPECT_EQ(sm[k2], 99);
}

TEST(SlotMapCustomGenTest, Uint32Generation) {
    SlotMap<double, uint32_t> sm;
    auto k = sm.insert(3.14);
    EXPECT_DOUBLE_EQ(sm[k], 3.14);
}

// ============================================================================
// slot_iter_from_data_iter
// ============================================================================

TEST_F(SlotMapTest, SlotIterFromDataIter) {
    auto k1 = sm.insert(10);
    sm.insert(20);
    sm.insert(30);

    auto data_it = sm.cbegin();
    auto slot_it = sm.slot_iter_from_data_iter(data_it);
    // The slot iterator should point to a valid slot whose data index
    // matches the data iterator position
    EXPECT_EQ(sm.data[slot_it->index], *data_it);
}

// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
