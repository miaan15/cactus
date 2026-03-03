#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <print>

import Ecs;

using namespace cactus::ecs;

struct Position {
    float x{}, y{}, z{};
};
struct Velocity {
    float vx{}, vy{}, vz{};
};
struct Health {
    int hp{100};
};
struct Tag {
    bool active{false};
};
struct BigData {
    std::array<double, 16> mat{};
};
struct ByteComp {
    uint8_t val{};
};
struct ShortComp {
    uint16_t val{};
};
struct Int64Comp {
    int64_t val{};
};
struct FloatComp {
    float val{};
};

TEST(EntityCreation, CreateEntityReturnsUniqueIds) {
    World w;
    auto e1 = w.create_entity();
    auto e2 = w.create_entity();
    auto e3 = w.create_entity();
    EXPECT_NE(e1, e2);
    EXPECT_NE(e2, e3);
    EXPECT_NE(e1, e3);
}

TEST(EntityCreation, ManyEntitiesAreUnique) {
    World w;
    constexpr int N = 256;
    std::vector<Entity> ids;
    ids.reserve(N);
    for (int i = 0; i < N; ++i) ids.push_back(w.create_entity());

    std::sort(ids.begin(), ids.end());
    EXPECT_EQ(std::unique(ids.begin(), ids.end()), ids.end());
}

TEST(EmplaceGet, singleComponent_Position) {
    World w;
    auto e = w.create_entity();
    w.emplace_component<Position>(e, 1.f, 2.f, 3.f);

    std::println("{}", w.get_debug());

    auto opt = w.get_component<Position>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_FLOAT_EQ(opt.value()->x, 1.f);
    EXPECT_FLOAT_EQ(opt.value()->y, 2.f);
    EXPECT_FLOAT_EQ(opt.value()->z, 3.f);
}

TEST(EmplaceGet, singleComponent_Health) {
    World w;
    auto e = w.create_entity();
    w.emplace_component<Health>(e, 42);

    std::println("{}", w.get_debug());

    auto opt = w.get_component<Health>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt.value()->hp, 42);
}

TEST(EmplaceGet, twoComponents_PositionAndVelocity) {
    World w;
    auto e = w.create_entity();
    w.emplace_component<Position>(e, 10.f, 20.f, 30.f);
    w.emplace_component<Velocity>(e, -1.f, 0.f, 1.f);

    std::println("{}", w.get_debug());

    auto posOpt = w.get_component<Position>(e);
    auto velOpt = w.get_component<Velocity>(e);
    ASSERT_TRUE(posOpt.has_value());
    ASSERT_TRUE(velOpt.has_value());
    EXPECT_FLOAT_EQ(posOpt.value()->x, 10.f);
    EXPECT_FLOAT_EQ(velOpt.value()->vx, -1.f);
}

TEST(EmplaceGet, allThreeComponents) {
    World w;
    auto e = w.create_entity();
    w.emplace_component<Position>(e, 5.f, 5.f, 5.f);
    w.emplace_component<Velocity>(e, 1.f, 2.f, 3.f);
    w.emplace_component<Health>(e, 99);

    std::println("{}", w.get_debug());

    EXPECT_FLOAT_EQ(w.get_component<Position>(e).value()->x, 5.f);
    EXPECT_FLOAT_EQ(w.get_component<Velocity>(e).value()->vy, 2.f);
    EXPECT_EQ(w.get_component<Health>(e).value()->hp, 99);
}

TEST(EmplaceGet, emplaceOverwritesExisting) {
    World w;
    auto e = w.create_entity();
    w.emplace_component<Health>(e, 100);
    w.emplace_component<Health>(e, 50); // overwrite

    std::println("{}", w.get_debug());

    auto opt = w.get_component<Health>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt.value()->hp, 50);
}

TEST(EmplaceGet, defaultConstructedComponent) {
    World w;
    auto e = w.create_entity();
    w.emplace_component<Position>(e); // uses default {0,0,0}

    std::println("{}", w.get_debug());

    auto opt = w.get_component<Position>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_FLOAT_EQ(opt.value()->x, 0.f);
    EXPECT_FLOAT_EQ(opt.value()->y, 0.f);
    EXPECT_FLOAT_EQ(opt.value()->z, 0.f);
}

TEST(EmplaceGet, Align) {
    World w;
    auto e0 = w.create_entity();
    auto e1 = w.create_entity();
    w.emplace_component<bool>(e0, true);
    w.emplace_component<double>(e0, 1.f);
    w.emplace_component<char>(e0, 'a');
    w.emplace_component<float>(e0, 1.f);

    w.emplace_component<double>(e1, 2.f);
    w.emplace_component<float>(e1, 2.f);
    w.emplace_component<char>(e1, 'b');
    w.emplace_component<bool>(e1, false);

    std::println("{}", w.get_debug());

    w.erase_component<bool>(e0);
    w.erase_component<float>(e0);

    std::println("{}", w.get_debug());

    w.erase_component<bool>(e1);
    w.erase_component<float>(e1);

    std::println("{}", w.get_debug());

    w.emplace_component<double>(e0, 5.f);
    w.emplace_component<float>(e0, 5.f);

    std::println("{}", w.get_debug());
}

// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
