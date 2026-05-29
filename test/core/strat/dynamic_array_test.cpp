#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

import std;
import cactus.core.strat;

using namespace cactus;

TEST_CASE("DynamicArray_BasicOperations") {
    auto arr = DynamicArray<int>::make();

    SUBCASE("Create and empty") {
        CHECK(arr.len == 0);
        CHECK(arr.cap == 0);
        CHECK(arr.empty());
    }

    SUBCASE("Append and get") {
        arr.append(10);
        arr.append(20);
        arr.append(30);

        CHECK(arr.len == 3);
        CHECK(arr.get(0).value() == 10);
        CHECK(arr.get(1).value() == 20);
        CHECK(arr.get(2).value() == 30);
        CHECK_FALSE(arr.empty());
    }

    SUBCASE("Set and get_ptr") {
        arr.append(100);
        arr.append(200);

        arr.set(0, 999);
        CHECK(arr.get(0).value() == 999);

        int *ptr = arr.get_ptr(1);
        CHECK(ptr != nullptr);
        CHECK(*ptr == 200);
    }

    SUBCASE("Pop and clear") {
        arr.append(1);
        arr.append(2);
        arr.append(3);

        arr.pop();
        CHECK(arr.len == 2);
        CHECK(arr.get(2) == std::nullopt);

        arr.clear();
        CHECK(arr.len == 0);
        CHECK(arr.empty());
    }

    SUBCASE("Reserve and resize") {
        arr.reserve(10);
        CHECK(arr.cap >= 10);

        arr.resize(5);
        CHECK(arr.len == 5);
    }

    SUBCASE("Iterator") {
        arr.append(10);
        arr.append(20);
        arr.append(30);

        int sum = 0;
        for (auto val : arr) { sum += val; }
        CHECK(sum == 60);
    }

    arr.destroy();
}

TEST_CASE("DynamicArray_StressTest") {
    auto arr = DynamicArray<int>::make();

    SUBCASE("Large append and growth") {
        for (int i = 0; i < 10000; i++) { arr.append(i); }

        CHECK(arr.len == 10000);
        CHECK(arr.get(0).value() == 0);
        CHECK(arr.get(9999).value() == 9999);

        int sum = 0;
        for (auto val : arr) { sum += val; }
        CHECK(sum == 10000 * 9999 / 2);
    }

    SUBCASE("Random access and modification") {
        for (int i = 0; i < 1000; i++) { arr.append(i * 2); }

        for (int i = 0; i < 1000; i++) { CHECK(arr.get(i).value() == i * 2); }

        for (int i = 0; i < 1000; i += 2) { arr.set(i, i * 3); }

        for (int i = 0; i < 1000; i += 2) { CHECK(arr.get(i).value() == i * 3); }
    }

    SUBCASE("Repeated pop and push") {
        for (int i = 0; i < 100; i++) { arr.append(i); }

        for (int round = 0; round < 10; round++) {
            for (int i = 0; i < 50; i++) { arr.pop(); }
            CHECK(arr.len == 50);

            for (int i = 0; i < 50; i++) { arr.append(1000 + round * 50 + i); }
            CHECK(arr.len == 100);
        }

        CHECK(arr.get(99).value() == 1000 + 9 * 50 + 49);
    }

    SUBCASE("Clone stress") {
        for (int i = 0; i < 5000; i++) { arr.append(i); }

        auto cloned = arr.clone();
        CHECK(cloned.len == arr.len);
        CHECK(cloned.cap == arr.cap);

        for (int i = 0; i < 5000; i++) { CHECK(cloned.get(i).value() == i); }

        cloned.append(9999);
        CHECK(arr.get(5000) == std::nullopt);
        CHECK(cloned.get(5000).value() == 9999);

        cloned.destroy();
    }

    arr.destroy();
}
