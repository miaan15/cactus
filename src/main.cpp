#include <gtest/gtest.h>

import Pool;

using P = cactus::Pool<double>;

TEST(Pool, StartsEmpty) {
    P p;
    EXPECT_TRUE(p.empty());
    EXPECT_EQ(p.size(), 0u);
}

TEST(Pool, InsertGrowsRawStorage) {
    P p;
    p.insert(10);
    p.insert(20);
    p.insert(30);
    EXPECT_EQ(p.size(), 3u);
    EXPECT_EQ(**p.at(0), 10);
    EXPECT_EQ(**p.at(1), 20);
    EXPECT_EQ(**p.at(2), 30);
}

TEST(Pool, AtReturnsNulloptOutOfRangeOrErased) {
    P p;
    EXPECT_FALSE(p.at(0).has_value());
    p.insert(7);
    p.erase(0u);
    EXPECT_FALSE(p.at(0).has_value());
    EXPECT_FALSE(p.at(99).has_value());
}

TEST(Pool, AtPointerAllowsMutation) {
    P p;
    p.insert(1);
    **p.at(0) = 99;
    EXPECT_EQ(**p.at(0), 99);
}

TEST(Pool, EraseByIndexDoesNotShrinkStorage) {
    P p;
    p.insert(1);
    p.insert(2);
    p.erase(0u);
    EXPECT_EQ(p.size(), 2u); // raw slot count unchanged
    EXPECT_FALSE(p.at(0).has_value());
    EXPECT_EQ(**p.at(1), 2);
}

TEST(Pool, EraseByIterator) {
    P p;
    p.insert(5);
    p.insert(6);
    p.erase(p.begin());
    EXPECT_FALSE(p.at(0).has_value());
    EXPECT_EQ(**p.at(1), 6);
}

TEST(Pool, FreeListReusesLIFO) {
    P p;
    p.insert(0);
    p.insert(1);
    p.insert(2); // slots 0,1,2
    p.erase(0u);
    p.erase(2u); // free list: 2 -> 0
    auto a = p.insert(10);
    auto b = p.insert(20);
    EXPECT_EQ(a._index, 2); // last erased = first reused
    EXPECT_EQ(b._index, 0);
    EXPECT_EQ(p.size(), 3u); // no extra allocation
}

TEST(Pool, ClearResetsToEmpty) {
    P p;
    p.insert(1);
    p.insert(2);
    p.erase(0u);
    p.clear();
    EXPECT_TRUE(p.empty());
    auto it = p.insert(42);
    EXPECT_EQ(it._index, 0); // fresh insert starts at slot 0
}

TEST(Pool, ReservePreallocatesWithoutInsert) {
    P p;
    p.reserve(64);
    EXPECT_TRUE(p.empty());
    EXPECT_GE(p.capacity(), 64u);
}

TEST(Pool, SwapExchangesContents) {
    P a, b;
    a.insert(1);
    a.insert(2);
    b.insert(9);
    cactus::swap(a, b);
    EXPECT_EQ(a.size(), 1u);
    EXPECT_EQ(**a.at(0), 9);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(**b.at(0), 1);
}

TEST(Pool, IteratorSkipsErasedSlots) {
    P p;
    p.insert(10);
    p.insert(20);
    p.insert(30);
    p.erase(1u);
    std::vector<int> live;
    for (int v : p) live.push_back(v);
    EXPECT_EQ(live, (std::vector<int>{10, 30}));
}

TEST(Pool, ReverseIteratorVisitsLiveValuesBackwards) {
    P p;
    p.insert(1);
    p.insert(2);
    p.insert(3);
    p.erase(1u); // erase middle
    std::vector<int> rev;
    for (auto it = p.rbegin(); it != p.rend(); ++it) rev.push_back(*it);
    EXPECT_EQ(rev, (std::vector<int>{3, 1}));
}

// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// #include <boost/container/vector.hpp>
// #include <cstdlib>
// #include <ctime>
//
// namespace rl {
// #include <raylib.h>
// }
//
// import Physics;
// import glm;
//
// using namespace glm;
// using namespace cactus;
// namespace bstc = boost::container;
//
// // Demo parameters
// constexpr int SCREEN_WIDTH = 1280;
// constexpr int SCREEN_HEIGHT = 720;
// constexpr int SCREEN_FPS = 144;
//
// constexpr int BOX_COUNT = 360;
// constexpr float MIN_BOX_SIZE = 5.0;
// constexpr float MAX_BOX_SIZE = 10.0;
// constexpr float MIN_BOX_VEL = 1000.0;
// constexpr float MAX_BOX_VEL = 5000.0;
// constexpr float BOX_RESTITUTION = 0.9;
// constexpr float BOX_FRICTION = 0.1;
//
// constexpr auto COUNT = 3636;
//
// auto random_float(float min, float max) -> float {
//     float normalized = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
//     return min + normalized * (max - min);
// }
//
// auto random_color() -> rl::Color {
//     return rl::Color{static_cast<unsigned char>(rand() % 256),
//                      static_cast<unsigned char>(rand() % 256),
//                      static_cast<unsigned char>(rand() % 256), 255};
// }
//
// auto main() -> int {
//     rl::InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cactus Physics Demo");
//     rl::SetTargetFPS(SCREEN_FPS);
//     srand(static_cast<unsigned>(time(nullptr)));
//
//     PhysicsWorld world{};
//     world.margin = 2.0f;
//
//     struct BoxData {
//         ColliderKey key;
//         rl::Color color;
//     };
//     bstc::vector<BoxData> boxes;
//
//     for (int i = 0; i < BOX_COUNT; i++) {
//         float halfsize = random_float(MIN_BOX_SIZE, MAX_BOX_SIZE) * 0.5f;
//         vec2 center{random_float(halfsize, SCREEN_WIDTH - halfsize),
//                     random_float(halfsize, SCREEN_HEIGHT - halfsize)};
//         vec2 vel{random_float(-MAX_BOX_VEL, MAX_BOX_VEL), random_float(-MAX_BOX_VEL,
//         MAX_BOX_VEL)};
//
//         if (length(vel) < MIN_BOX_VEL) {
//             vel = normalize(vel) * MIN_BOX_VEL;
//         }
//
//         float invmass = 1.0f / (halfsize * halfsize * 4.0f);
//
//         ColliderKey key = world.create(center, vec2(halfsize, halfsize), invmass,
//         BOX_RESTITUTION,
//                                        BOX_FRICTION, BOX_FRICTION);
//         auto entry = world.get(key);
//         entry->vel = vel;
//
//         boxes.push_back({key, random_color()});
//     }
//
//     size_t cur_count = 0;
//     bool pause = false;
//     size_t left = 0, right = 0;
//     while (!rl::WindowShouldClose()) {
//         if (!pause) {
//             float dt = rl::GetFrameTime();
//
//             for (auto &box : boxes) {
//                 world.get(box.key)->vel.y -= -9.81 * 10 * dt;
//             }
//
//             world.update(dt);
//
//             for (auto &box : boxes) {
//                 auto entry = world.get(box.key);
//                 vec2 halfexts = entry->coll.halfexts;
//
//                 if (entry->coll.center.x - halfexts.x < 0) {
//                     entry->coll.center.x = halfexts.x;
//                     entry->vel.x *= -1;
//                 }
//                 if (entry->coll.center.x + halfexts.x > SCREEN_WIDTH) {
//                     entry->coll.center.x = SCREEN_WIDTH - halfexts.x;
//                     entry->vel.x *= -1;
//                 }
//                 if (entry->coll.center.y - halfexts.y < 0) {
//                     entry->coll.center.y = halfexts.y;
//                     entry->vel.y *= -1;
//                 }
//                 if (entry->coll.center.y + halfexts.y > SCREEN_HEIGHT) {
//                     entry->coll.center.y = SCREEN_HEIGHT - halfexts.y;
//                     entry->vel.y *= -1;
//                 }
//             }
//             if (++cur_count > COUNT) {
//                 pause = true;
//
//                 for (auto &box : boxes) {
//                     if (world.get(box.key)->coll.center.x < (float)SCREEN_WIDTH / 2) left++;
//                     else right++;
//                 }
//             }
//         }
//
//         rl::BeginDrawing();
//         rl::ClearBackground(rl::RAYWHITE);
//
//         for (const auto &box : boxes) {
//             auto entry = world.get(box.key);
//             vec2 center = entry->coll.center;
//             vec2 halfexts = entry->coll.halfexts;
//
//             rl::DrawRectangle(
//                 static_cast<int>(center.x - halfexts.x), static_cast<int>(center.y - halfexts.y),
//                 static_cast<int>(halfexts.x * 2), static_cast<int>(halfexts.y * 2), box.color);
//
//             rl::DrawRectangleLines(
//                 static_cast<int>(center.x - halfexts.x), static_cast<int>(center.y - halfexts.y),
//                 static_cast<int>(halfexts.x * 2), static_cast<int>(halfexts.y * 2), rl::BLACK);
//         }
//
//         // rl::DrawFPS(10, 10);
//
//         if (!pause) {
//             rl::DrawText(rl::TextFormat("%d", (int)((1.0 - (float)cur_count / COUNT) * 100.0)),
//                          SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 300, 100, rl::BLACK);
//         }
//         if (pause) {
//             rl::DrawText(rl::TextFormat("%d", left), 30, SCREEN_HEIGHT / 2 - 50, 100, rl::BLACK);
//             rl::DrawText(rl::TextFormat("%d", right), SCREEN_WIDTH / 2 + 30, SCREEN_HEIGHT / 2 -
//             50,
//                          100, rl::BLACK);
//         }
//
//         rl::EndDrawing();
//     }
//
//     rl::CloseWindow();
//
//     return 0;
// }

// #include <algorithm>
// #include <cstdint>
// #include <gtest/gtest.h>
// #include <string>
// #include <vector>
//
// import SlotMap;
//
// using cactus::get_idx;
// using cactus::get_gen;
// using cactus::SlotMap;
//
// // ---------------------------------------------------------------------------
// // Helper key utilities
// // ---------------------------------------------------------------------------
//
//
// TEST(SlotMapKeyHelpers, GetIdxReturnsUpper32Bits) {
//     uint64_t key = 0x0000'0003'0000'0005ULL;
//     EXPECT_EQ(get_idx(key), 3u);
// }
//
// TEST(SlotMapKeyHelpers, GetGenReturnsLower32Bits) {
//     uint64_t key = 0x0000'0003'0000'0005ULL;
//     EXPECT_EQ(get_gen(key), 5u);
// }
//
// // ---------------------------------------------------------------------------
// // Construction & empty state
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, DefaultConstructedIsEmpty) {
//     SlotMap<int> sm{};
//     EXPECT_TRUE(sm.empty());
//     EXPECT_EQ(sm.size(), 0u);
// }
//
// // ---------------------------------------------------------------------------
// // Insert / find / at
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, InsertSingleElement) {
//     SlotMap<int> sm{};
//     auto key = sm.insert(42);
//
//     EXPECT_EQ(sm.size(), 1u);
//     EXPECT_FALSE(sm.empty());
//
//     auto it = sm.find(key);
//     ASSERT_NE(it, sm.end());
//     EXPECT_EQ(*it, 42);
// }
//
// TEST(SlotMapTest, InsertMultipleElements) {
//     SlotMap<std::string> sm{};
//     auto k1 = sm.insert("alpha");
//     auto k2 = sm.insert("beta");
//     auto k3 = sm.insert("gamma");
//
//     EXPECT_EQ(sm.size(), 3u);
//
//     EXPECT_EQ(*sm.find(k1), "alpha");
//     EXPECT_EQ(*sm.find(k2), "beta");
//     EXPECT_EQ(*sm.find(k3), "gamma");
// }
//
// TEST(SlotMapTest, AtReturnsValueWhenKeyValid) {
//     SlotMap<int> sm{};
//     auto key = sm.insert(99);
//
//     auto opt = sm.at(key);
//     ASSERT_TRUE(opt.has_value());
//     EXPECT_EQ(*opt.value(), 99);
// }
//
// TEST(SlotMapTest, AtReturnsNulloptWhenKeyInvalid) {
//     SlotMap<int> sm{};
//     sm.insert(10);
//
//     uint64_t bogus_key = 0xDEAD'BEEF'0000'00FFULL;
//     EXPECT_FALSE(sm.at(bogus_key).has_value());
// }
//
// TEST(SlotMapTest, FindReturnsEndForOutOfRangeIndex) {
//     SlotMap<int> sm{};
//     sm.insert(1);
//
//     uint64_t bad_key = 0x0000'0000'0000'00FFULL; // sparse idx way out of range
//     EXPECT_EQ(sm.find(bad_key), sm.end());
// }
//
// // ---------------------------------------------------------------------------
// // Erase by key
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, EraseByKeyRemovesElement) {
//     SlotMap<int> sm{};
//     auto k1 = sm.insert(10);
//     auto k2 = sm.insert(20);
//     auto k3 = sm.insert(30);
//
//     EXPECT_EQ(sm.erase(k2), 1u);
//     EXPECT_EQ(sm.size(), 2u);
//     EXPECT_EQ(sm.find(k2), sm.end());
//
//     // remaining elements still reachable
//     EXPECT_NE(sm.find(k1), sm.end());
//     EXPECT_NE(sm.find(k3), sm.end());
// }
//
// TEST(SlotMapTest, EraseByInvalidKeyReturnsZero) {
//     SlotMap<int> sm{};
//     sm.insert(1);
//
//     uint64_t bogus = 0xBAD0'0000'0000'0000ULL;
//     EXPECT_EQ(sm.erase(bogus), 0u);
// }
//
// TEST(SlotMapTest, ErasedKeyIsInvalidatedByGeneration) {
//     SlotMap<int> sm{};
//     auto key = sm.insert(42);
//     sm.erase(key);
//
//     // same slot may be reused, but the old key must not resolve
//     EXPECT_EQ(sm.find(key), sm.end());
//     EXPECT_FALSE(sm.at(key).has_value());
// }
//
// // ---------------------------------------------------------------------------
// // Erase by iterator
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, EraseByIterator) {
//     SlotMap<int> sm{};
//     sm.insert(1);
//     sm.insert(2);
//     sm.insert(3);
//
//     auto it = sm.begin();
//     sm.erase(it);
//
//     EXPECT_EQ(sm.size(), 2u);
// }
//
// // ---------------------------------------------------------------------------
// // Slot reuse after erase
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, SlotIsReusedAfterErase) {
//     SlotMap<int> sm{};
//     auto k1 = sm.insert(100);
//     sm.erase(k1);
//
//     auto k2 = sm.insert(200);
//     EXPECT_EQ(sm.size(), 1u);
//     EXPECT_EQ(*sm.find(k2), 200);
//
//     // old key must still be invalid
//     EXPECT_EQ(sm.find(k1), sm.end());
// }
//
// // ---------------------------------------------------------------------------
// // Clear
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, ClearRemovesAllElements) {
//     SlotMap<int> sm{};
//     sm.insert(1);
//     sm.insert(2);
//     sm.insert(3);
//
//     sm.clear();
//
//     EXPECT_TRUE(sm.empty());
//     EXPECT_EQ(sm.size(), 0u);
//     EXPECT_EQ(sm.begin(), sm.end());
// }
//
// // ---------------------------------------------------------------------------
// // Reserve
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, ReserveDoesNotChangeSize) {
//     SlotMap<int> sm{};
//     sm.reserve(100);
//     EXPECT_TRUE(sm.empty());
// }
//
// // ---------------------------------------------------------------------------
// // Iteration
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, RangeBasedForVisitsAllElements) {
//     SlotMap<int> sm{};
//     sm.insert(10);
//     sm.insert(20);
//     sm.insert(30);
//
//     int sum = 0;
//     for (auto &v : sm) {
//         sum += v;
//     }
//     EXPECT_EQ(sum, 60);
// }
//
// TEST(SlotMapTest, ReverseIteratorsWork) {
//     SlotMap<int> sm{};
//     sm.insert(1);
//     sm.insert(2);
//     sm.insert(3);
//
//     std::vector<int> reversed(sm.rbegin(), sm.rend());
//     ASSERT_EQ(reversed.size(), 3u);
//     // reverse of insertion order
//     EXPECT_EQ(reversed[0], 3);
//     EXPECT_EQ(reversed[1], 2);
//     EXPECT_EQ(reversed[2], 1);
// }
//
// // ---------------------------------------------------------------------------
// // Swap
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, SwapExchangesContents) {
//     SlotMap<int> a{};
//     SlotMap<int> b{};
//
//     auto ka = a.insert(1);
//     b.insert(100);
//     b.insert(200);
//
//     swap(a, b);
//
//     EXPECT_EQ(a.size(), 2u);
//     EXPECT_EQ(b.size(), 1u);
//     EXPECT_NE(b.find(ka), b.end());
// }
//
// // ---------------------------------------------------------------------------
// // Const access
// // ---------------------------------------------------------------------------
//
// TEST(SlotMapTest, ConstFindAndAt) {
//     SlotMap<int> sm{};
//     auto key = sm.insert(77);
//
//     const auto &csm = sm;
//
//     auto it = csm.find(key);
//     ASSERT_NE(it, csm.end());
//     EXPECT_EQ(*it, 77);
//
//     auto opt = csm.at(key);
//     ASSERT_TRUE(opt.has_value());
//     EXPECT_EQ(*opt.value(), 77);
// }
//
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
