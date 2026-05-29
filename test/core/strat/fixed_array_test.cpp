#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

import std;
import cactus.core.strat;

using namespace cactus;

TEST_CASE("FixedArray_BasicOperations") {
    auto arr = FixedArray<int>::make(10);

    SUBCASE("Create and empty check") {
        CHECK(arr.len == 10);

        for (size_t i = 0; i < 10; i++) { CHECK(arr.get(i) == 0); }
    }

    SUBCASE("Set and get") {
        arr.set(0, 100);
        arr.set(2, 200);
        arr.set(4, 300);

        CHECK(arr.get(0).value() == 100);
        CHECK(arr.get(1).value() == 0);
        CHECK(arr.get(2).value() == 200);
        CHECK(arr.get(4).value() == 300);
    }

    SUBCASE("Get pointer") {
        arr.set(0, 42);

        int *ptr = arr.get_ptr(0);
        CHECK(ptr != nullptr);
        CHECK(*ptr == 42);

        const int *cptr = static_cast<const FixedArray<int> &>(arr).get_ptr(0);
        CHECK(cptr != nullptr);
        CHECK(*cptr == 42);
    }

    SUBCASE("Iterator") {
        for (size_t i = 0; i < 5; i++) { arr.set(i, static_cast<int>(i * 10)); }

        int sum = 0;
        for (auto val : arr) { sum += val; }
        CHECK(sum == 100); // 0 + 10 + 20 + 30 + 40
    }

    SUBCASE("Out of bounds") {
        CHECK(arr.get(10) == std::nullopt);
        CHECK(arr.get_ptr(10) == nullptr);
        CHECK_FALSE(arr.set(10, 999));
    }

    arr.destroy();
}

TEST_CASE("FixedArray_StressTest") {
    SUBCASE("Large array fill") {
        auto arr = FixedArray<int>::make(10000);
        for (int i = 0; i < 10000; i++) { arr.set(i, i * 3); }

        int sum = 0;
        for (auto val : arr) { sum += val; }
        CHECK(sum == 3 * 10000 * 9999 / 2);
        arr.destroy();
    }

    SUBCASE("Random access pattern") {
        auto arr = FixedArray<int>::make(5000);
        for (int i = 0; i < 5000; i++) { arr.set(i, i); }

        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(0, 4999);

        for (int i = 0; i < 1000; i++) {
            int idx = dist(rng);
            CHECK(arr.get(idx).value() == idx);
        }
        arr.destroy();
    }

    SUBCASE("Clone stress") {
        auto arr = FixedArray<int>::make(3000);
        for (int i = 0; i < 3000; i++) { arr.set(i, i * 7); }

        auto cloned = arr.clone();
        CHECK(cloned.len == arr.len);

        arr.destroy();
        cloned.destroy();
    }

    SUBCASE("Remake resize") {
        auto arr = FixedArray<int>::make(100);
        for (int i = 0; i < 100; i++) { arr.set(i, i); }

        arr.remake(50);
        CHECK(arr.len == 50);
        for (size_t i = 0; i < 50; i++) { CHECK(arr.get(i) == i); }

        arr.remake(200);
        CHECK(arr.len == 200);
        for (size_t i = 0; i < 50; i++) { CHECK(arr.get(i) == i); }
        for (size_t i = 50; i < 200; i++) { CHECK(arr.get(i) == 0); }

        arr.destroy();
    }
}
