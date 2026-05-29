#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

import std;
import cactus.core.strat;

using namespace cactus;

TEST_CASE("HashMap_BasicOperations") {
    SUBCASE("Create and empty") {
        auto map = HashMap<int, int>::make();
        CHECK(map.len == 0);
        CHECK(map.cap == 0);
        CHECK(map.empty());
        map.destroy();
    }

    SUBCASE("Add and get") {
        auto map = HashMap<int, int>::make();
        map.add(1, 100);
        map.add(2, 200);
        map.add(3, 300);

        CHECK(map.len == 3);
        CHECK(map.get(1).value() == 100);
        CHECK(map.get(2).value() == 200);
        CHECK(map.get(3).value() == 300);
        map.destroy();
    }

    SUBCASE("Get pointer and modify") {
        auto map = HashMap<int, int>::make();
        map.add(42, 10);

        int *ptr = map.get_ptr(42);
        CHECK(ptr != nullptr);
        *ptr = 999;
        CHECK(map.get(42).value() == 999);
        map.destroy();
    }

    SUBCASE("Get or add") {
        auto map = HashMap<int, int>::make();
        int *ptr1 = map.get_or_add_ptr(1);
        *ptr1 = 100;

        int *ptr2 = map.get_or_add_ptr(1);
        CHECK(ptr1 == ptr2);
        CHECK(*ptr2 == 100);

        int *ptr3 = map.get_or_add_ptr(2);
        CHECK(ptr3 != ptr1);
        CHECK(map.get(2).value() == 0);
        map.destroy();
    }

    SUBCASE("Has and remove") {
        auto map = HashMap<int, int>::make();
        map.add(1, 100);

        CHECK(map.has(1));
        CHECK_FALSE(map.has(2));

        CHECK(map.remove(1) == true);
        CHECK_FALSE(map.has(1));
        CHECK(map.remove(1) == false);
        map.destroy();
    }

    SUBCASE("Clear") {
        auto map = HashMap<int, int>::make();
        map.add(1, 100);
        map.add(2, 200);
        map.add(3, 300);

        map.clear();
        CHECK(map.len == 0);
        CHECK(map.empty());
        CHECK_FALSE(map.has(1));
        map.destroy();
    }

    SUBCASE("Iterator") {
        auto map = HashMap<int, int>::make();
        map.add(1, 10);
        map.add(2, 20);
        map.add(3, 30);

        int sum = 0;
        for (const auto &[key, val] : map) {
            sum += val;
        }
        CHECK(sum == 60);
        map.destroy();
    }

    SUBCASE("Update existing key") {
        auto map = HashMap<int, int>::make();
        map.add(1, 100);
        map.add(1, 200);
        CHECK(map.len == 1);
        CHECK(map.get(1).value() == 200);
        map.destroy();
    }
}

TEST_CASE("HashMap_StressTest") {
    SUBCASE("Large insert and rehash") {
        auto map = HashMap<int, int>::make();
        for (int i = 0; i < 10000; i++) {
            map.add(i, i * 2);
        }

        CHECK(map.len == 10000);
        CHECK(map.cap >= 10000);

        for (int i = 0; i < 10000; i++) {
            CHECK(map.get(i).value() == i * 2);
        }
        map.destroy();
    }

    SUBCASE("Random operations") {
        auto map = HashMap<int, int>::make();
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> key_dist(0, 999);
        std::uniform_int_distribution<int> val_dist(0, 10000);

        for (int i = 0; i < 5000; i++) {
            int key = key_dist(rng);
            int val = val_dist(rng);
            map.add(key, val);
        }

        int sum = 0;
        for (const auto &[key, val] : map) {
            sum += val;
        }
        CHECK(sum > 0);
        map.destroy();
    }

    SUBCASE("Remove and re-add stress") {
        auto map = HashMap<int, int>::make();
        for (int i = 0; i < 1000; i++) {
            map.add(i, i);
        }

        for (int i = 0; i < 500; i++) {
            CHECK(map.remove(i) == true);
        }
        CHECK(map.len == 500);

        for (int i = 0; i < 500; i++) {
            map.add(i, i * 2);
        }
        CHECK(map.len == 1000);
        CHECK(map.get(0).value() == 0 * 2);
        CHECK(map.get(499).value() == 499 * 2);
        map.destroy();
    }

    SUBCASE("Clone stress") {
        auto map = HashMap<int, int>::make();
        for (int i = 0; i < 5000; i++) {
            map.add(i, i * 7);
        }

        auto cloned = map.clone();
        CHECK(cloned.len == map.len);

        for (int i = 0; i < 5000; i++) {
            CHECK(cloned.get(i).value() == i * 7);
        }

        cloned.add(10000, 99999);
        CHECK(map.get(10000) == std::nullopt);
        CHECK(cloned.get(10000).value() == 99999);

        cloned.destroy();
        map.destroy();
    }

    SUBCASE("String keys") {
        auto map = HashMap<std::string, int>::make();
        map.add("hello", 1);
        map.add("world", 2);
        map.add("test", 3);

        CHECK(map.len == 3);
        CHECK(map.get("hello").value() == 1);
        CHECK(map.get("world").value() == 2);
        CHECK(map.get("test").value() == 3);
        CHECK(map.get("nonexistent") == std::nullopt);
        map.destroy();
    }
}
