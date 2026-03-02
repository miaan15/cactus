#include <array>
#include <cstdint>
#include <gtest/gtest.h>

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

// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
