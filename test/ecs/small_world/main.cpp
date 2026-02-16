#include <array>
#include <cstdint>
#include <gtest/gtest.h>

import Ecs;

using namespace cactus::ecs;

// ============================================================================
//  Helper component types used across many test groups
// ============================================================================

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

// ============================================================================
// 1. Compile-time / static tests
// ============================================================================

TEST(StaticTests, component_id_uniquePerType) {
    using W = SmallWorld<Position, Velocity, Health>;
    constexpr auto posId = W::component_id<Position>();
    constexpr auto velId = W::component_id<Velocity>();
    constexpr auto hpId = W::component_id<Health>();

    EXPECT_EQ(posId, 0u);
    EXPECT_EQ(velId, 1u);
    EXPECT_EQ(hpId, 2u);
    EXPECT_NE(posId, velId);
    EXPECT_NE(velId, hpId);
    EXPECT_NE(posId, hpId);
}

TEST(StaticTests, is_components_contain_returnsTrueForPresent) {
    using W = SmallWorld<Position, Velocity, Health>;
    EXPECT_TRUE(W::is_components_contain<Position>());
    EXPECT_TRUE(W::is_components_contain<Velocity>());
    EXPECT_TRUE(W::is_components_contain<Health>());
}

TEST(StaticTests, is_components_contain_returnsFalseForAbsent) {
    using W = SmallWorld<Position, Velocity>;
    EXPECT_FALSE(W::is_components_contain<Health>());
    EXPECT_FALSE(W::is_components_contain<Tag>());
}

TEST(StaticTests, isUniqueConstraintEnforced) {
    // SmallWorld<Position, Position> must not compile — verified via
    // static_assert in real code; here we check the helper directly.
    EXPECT_TRUE((is_unique_v<int, float, double>));
    EXPECT_FALSE((is_unique_v<int, float, int>));
    EXPECT_TRUE((is_unique_v<Position, Velocity, Health>));
    EXPECT_FALSE((is_unique_v<Position, Position>));
}

TEST(StaticTests, singleComponentWorld) {
    using W = SmallWorld<Position>;
    EXPECT_EQ(W::component_id<Position>(), 0u);
    EXPECT_TRUE(W::is_components_contain<Position>());
}

// ============================================================================
// 2. Entity creation
// ============================================================================

TEST(EntityCreation, createEntityReturnsUniqueIds) {
    SmallWorld<Position, Velocity> w;
    auto e1 = w.create_entity();
    auto e2 = w.create_entity();
    auto e3 = w.create_entity();
    EXPECT_NE(e1, e2);
    EXPECT_NE(e2, e3);
    EXPECT_NE(e1, e3);
}

TEST(EntityCreation, manyEntitiesAreUnique) {
    SmallWorld<Position> w;
    constexpr int N = 256;
    std::vector<Entity> ids;
    ids.reserve(N);
    for (int i = 0; i < N; ++i) ids.push_back(w.create_entity());

    // All unique
    std::sort(ids.begin(), ids.end());
    EXPECT_EQ(std::unique(ids.begin(), ids.end()), ids.end());
}

// ============================================================================
// 3. get() — component not yet present
// ============================================================================

TEST(GetComponent, returnsNulloptWhenComponentAbsent) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    EXPECT_FALSE(w.get<Position>(e).has_value());
    EXPECT_FALSE(w.get<Velocity>(e).has_value());
    EXPECT_FALSE(w.get<Health>(e).has_value());
}

// ============================================================================
// 4. emplace() + get() round-trips
// ============================================================================

TEST(EmplaceGet, singleComponent_Position) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Position>(e, 1.f, 2.f, 3.f);

    auto opt = w.get<Position>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_FLOAT_EQ(opt.value()->x, 1.f);
    EXPECT_FLOAT_EQ(opt.value()->y, 2.f);
    EXPECT_FLOAT_EQ(opt.value()->z, 3.f);
}

TEST(EmplaceGet, singleComponent_Health) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Health>(e, 42);

    auto opt = w.get<Health>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt.value()->hp, 42);
}

TEST(EmplaceGet, twoComponents_PositionAndVelocity) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Position>(e, 10.f, 20.f, 30.f);
    w.emplace<Velocity>(e, -1.f, 0.f, 1.f);

    auto posOpt = w.get<Position>(e);
    auto velOpt = w.get<Velocity>(e);
    ASSERT_TRUE(posOpt.has_value());
    ASSERT_TRUE(velOpt.has_value());
    EXPECT_FLOAT_EQ(posOpt.value()->x, 10.f);
    EXPECT_FLOAT_EQ(velOpt.value()->vx, -1.f);
}

TEST(EmplaceGet, allThreeComponents) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Position>(e, 5.f, 5.f, 5.f);
    w.emplace<Velocity>(e, 1.f, 2.f, 3.f);
    w.emplace<Health>(e, 99);

    EXPECT_FLOAT_EQ(w.get<Position>(e).value()->x, 5.f);
    EXPECT_FLOAT_EQ(w.get<Velocity>(e).value()->vy, 2.f);
    EXPECT_EQ(w.get<Health>(e).value()->hp, 99);
}

TEST(EmplaceGet, emplaceOverwritesExisting) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Health>(e, 100);
    w.emplace<Health>(e, 50); // overwrite

    auto opt = w.get<Health>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt.value()->hp, 50);
}

TEST(EmplaceGet, defaultConstructedComponent) {
    SmallWorld<Position, Health> w;
    auto e = w.create_entity();
    w.emplace<Position>(e); // uses default {0,0,0}

    auto opt = w.get<Position>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_FLOAT_EQ(opt.value()->x, 0.f);
    EXPECT_FLOAT_EQ(opt.value()->y, 0.f);
    EXPECT_FLOAT_EQ(opt.value()->z, 0.f);
}

// ============================================================================
// 5. erase() tests
// ============================================================================

TEST(EraseComponent, removedComponentIsNoLongerAccessible) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Health>(e, 77);
    ASSERT_TRUE(w.get<Health>(e).has_value());

    w.erase<Health>(e);
    EXPECT_FALSE(w.get<Health>(e).has_value());
}

TEST(EraseComponent, eraseNonExistentComponentIsNoOp) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    // Nothing emplaced — erase should silently do nothing
    EXPECT_NO_THROW(w.erase<Position>(e));
    EXPECT_FALSE(w.get<Position>(e).has_value());
}

TEST(EraseComponent, otherComponentsSurviveErase) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Position>(e, 1.f, 2.f, 3.f);
    w.emplace<Health>(e, 55);
    w.erase<Position>(e);

    EXPECT_FALSE(w.get<Position>(e).has_value());
    ASSERT_TRUE(w.get<Health>(e).has_value());
    EXPECT_EQ(w.get<Health>(e).value()->hp, 55);
}

TEST(EraseComponent, eraseAndReEmplace) {
    SmallWorld<Position, Health> w;
    auto e = w.create_entity();
    w.emplace<Health>(e, 100);
    w.erase<Health>(e);
    w.emplace<Health>(e, 200);

    auto opt = w.get<Health>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt.value()->hp, 200);
}

// ============================================================================
// 6. Multiple entities — isolation
// ============================================================================

TEST(MultiEntity, componentsAreIsolatedBetweenEntities) {
    SmallWorld<Position, Health> w;
    auto e1 = w.create_entity();
    auto e2 = w.create_entity();
    w.emplace<Health>(e1, 10);
    w.emplace<Health>(e2, 90);

    EXPECT_EQ(w.get<Health>(e1).value()->hp, 10);
    EXPECT_EQ(w.get<Health>(e2).value()->hp, 90);
}

TEST(MultiEntity, modifyOneDoesNotAffectOther) {
    SmallWorld<Position, Health> w;
    auto e1 = w.create_entity();
    auto e2 = w.create_entity();
    w.emplace<Health>(e1, 50);
    w.emplace<Health>(e2, 50);

    w.get<Health>(e1).value()->hp = 1; // mutate via pointer

    EXPECT_EQ(w.get<Health>(e1).value()->hp, 1);
    EXPECT_EQ(w.get<Health>(e2).value()->hp, 50);
}

TEST(MultiEntity, eraseOnOneDoesNotAffectOther) {
    SmallWorld<Position, Health> w;
    auto e1 = w.create_entity();
    auto e2 = w.create_entity();
    w.emplace<Health>(e1, 33);
    w.emplace<Health>(e2, 77);
    w.erase<Health>(e1);

    EXPECT_FALSE(w.get<Health>(e1).has_value());
    ASSERT_TRUE(w.get<Health>(e2).has_value());
    EXPECT_EQ(w.get<Health>(e2).value()->hp, 77);
}

TEST(MultiEntity, manyEntitiesWithSameSignature) {
    SmallWorld<Position, Health> w;
    constexpr int N = 64;
    std::vector<Entity> entities;
    entities.reserve(N);

    for (int i = 0; i < N; ++i) {
        auto e = w.create_entity();
        w.emplace<Health>(e, i);
        entities.push_back(e);
    }

    for (int i = 0; i < N; ++i) {
        auto opt = w.get<Health>(entities[i]);
        ASSERT_TRUE(opt.has_value()) << "Entity " << i << " missing Health";
        EXPECT_EQ(opt.value()->hp, i) << "Entity " << i << " wrong value";
    }
}

// ============================================================================
// 7. Diverse component types
// ============================================================================

TEST(DiverseTypes, uint8_component) {
    SmallWorld<ByteComp> w;
    auto e = w.create_entity();
    w.emplace<ByteComp>(e, uint8_t(255));
    ASSERT_TRUE(w.get<ByteComp>(e).has_value());
    EXPECT_EQ(w.get<ByteComp>(e).value()->val, 255u);
}

TEST(DiverseTypes, uint16_component) {
    SmallWorld<ShortComp> w;
    auto e = w.create_entity();
    w.emplace<ShortComp>(e, uint16_t(60000));
    ASSERT_TRUE(w.get<ShortComp>(e).has_value());
    EXPECT_EQ(w.get<ShortComp>(e).value()->val, 60000u);
}

TEST(DiverseTypes, int64_component) {
    SmallWorld<Int64Comp> w;
    auto e = w.create_entity();
    w.emplace<Int64Comp>(e, int64_t(-9000000000LL));
    ASSERT_TRUE(w.get<Int64Comp>(e).has_value());
    EXPECT_EQ(w.get<Int64Comp>(e).value()->val, -9000000000LL);
}

TEST(DiverseTypes, floatComponent_precision) {
    SmallWorld<FloatComp> w;
    auto e = w.create_entity();
    w.emplace<FloatComp>(e, 3.14159f);
    ASSERT_TRUE(w.get<FloatComp>(e).has_value());
    EXPECT_FLOAT_EQ(w.get<FloatComp>(e).value()->val, 3.14159f);
}

TEST(DiverseTypes, bigDataComponent_16Doubles) {
    SmallWorld<BigData, Position> w;
    auto e = w.create_entity();
    BigData bd;
    for (int i = 0; i < 16; ++i) bd.mat[i] = i * 1.5;
    w.emplace<BigData>(e, bd);

    auto opt = w.get<BigData>(e);
    ASSERT_TRUE(opt.has_value());
    for (int i = 0; i < 16; ++i)
        EXPECT_DOUBLE_EQ(opt.value()->mat[i], i * 1.5) << "mat[" << i << "] mismatch";
}

TEST(DiverseTypes, boolLikeTag) {
    SmallWorld<Tag, Position> w;
    auto e = w.create_entity();
    w.emplace<Tag>(e, true);
    ASSERT_TRUE(w.get<Tag>(e).has_value());
    EXPECT_TRUE(w.get<Tag>(e).value()->active);
}

TEST(DiverseTypes, mixedSmallAndLargeComponents) {
    SmallWorld<ByteComp, BigData, FloatComp> w;
    auto e = w.create_entity();
    w.emplace<ByteComp>(e, uint8_t(7));
    w.emplace<BigData>(e);
    w.emplace<FloatComp>(e, 2.71828f);

    EXPECT_EQ(w.get<ByteComp>(e).value()->val, 7u);
    EXPECT_FLOAT_EQ(w.get<FloatComp>(e).value()->val, 2.71828f);
}

// ============================================================================
// 8. World with maximum feasible component count (testing 64-component limit)
// ============================================================================

// We define 8 types and create a SmallWorld with 8 components as a
// representative stress test (defining 64 unique structs inline is verbose).
struct C0 {
    int v{};
};
struct C1 {
    int v{};
};
struct C2 {
    int v{};
};
struct C3 {
    int v{};
};
struct C4 {
    int v{};
};
struct C5 {
    int v{};
};
struct C6 {
    int v{};
};
struct C7 {
    int v{};
};

TEST(LargeWorld, eightComponentTypes_allEmplaceAndGet) {
    SmallWorld<C0, C1, C2, C3, C4, C5, C6, C7> w;
    auto e = w.create_entity();
    w.emplace<C0>(e, 0);
    w.emplace<C1>(e, 1);
    w.emplace<C2>(e, 2);
    w.emplace<C3>(e, 3);
    w.emplace<C4>(e, 4);
    w.emplace<C5>(e, 5);
    w.emplace<C6>(e, 6);
    w.emplace<C7>(e, 7);

    EXPECT_EQ(w.get<C0>(e).value()->v, 0);
    EXPECT_EQ(w.get<C1>(e).value()->v, 1);
    EXPECT_EQ(w.get<C2>(e).value()->v, 2);
    EXPECT_EQ(w.get<C3>(e).value()->v, 3);
    EXPECT_EQ(w.get<C4>(e).value()->v, 4);
    EXPECT_EQ(w.get<C5>(e).value()->v, 5);
    EXPECT_EQ(w.get<C6>(e).value()->v, 6);
    EXPECT_EQ(w.get<C7>(e).value()->v, 7);
}

TEST(LargeWorld, selectively_eraseHalfComponents) {
    SmallWorld<C0, C1, C2, C3, C4, C5, C6, C7> w;
    auto e = w.create_entity();
    w.emplace<C0>(e, 0);
    w.emplace<C1>(e, 1);
    w.emplace<C2>(e, 2);
    w.emplace<C3>(e, 3);
    w.emplace<C4>(e, 4);
    w.emplace<C5>(e, 5);
    w.emplace<C6>(e, 6);
    w.emplace<C7>(e, 7);

    w.erase<C1>(e);
    w.erase<C3>(e);
    w.erase<C5>(e);
    w.erase<C7>(e);

    EXPECT_TRUE(w.get<C0>(e).has_value());
    EXPECT_FALSE(w.get<C1>(e).has_value());
    EXPECT_TRUE(w.get<C2>(e).has_value());
    EXPECT_FALSE(w.get<C3>(e).has_value());
    EXPECT_TRUE(w.get<C4>(e).has_value());
    EXPECT_FALSE(w.get<C5>(e).has_value());
    EXPECT_TRUE(w.get<C6>(e).has_value());
    EXPECT_FALSE(w.get<C7>(e).has_value());
}

// ============================================================================
// 9. Signature / archetype transitions
// ============================================================================

TEST(Archetype, signatureChangesOnEmplace) {
    // Verify indirectly: after emplace, get() returns a value (new archetype)
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    EXPECT_FALSE(w.get<Position>(e).has_value()); // signature = 0
    w.emplace<Position>(e, 1.f, 0.f, 0.f);
    EXPECT_TRUE(w.get<Position>(e).has_value()); // signature = 0b001
    w.emplace<Velocity>(e, 0.f, 1.f, 0.f);
    EXPECT_TRUE(w.get<Velocity>(e).has_value()); // signature = 0b011
    EXPECT_TRUE(w.get<Position>(e).has_value()); // still present
}

TEST(Archetype, signatureChangesOnErase) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Position>(e, 9.f, 0.f, 0.f);
    w.emplace<Velocity>(e, 0.f, 9.f, 0.f);
    w.erase<Velocity>(e);

    EXPECT_FALSE(w.get<Velocity>(e).has_value());
    ASSERT_TRUE(w.get<Position>(e).has_value());
    EXPECT_FLOAT_EQ(w.get<Position>(e).value()->x, 9.f);
}

TEST(Archetype, fullCycle_addAllThenRemoveAll) {
    SmallWorld<Position, Velocity, Health> w;
    auto e = w.create_entity();
    w.emplace<Position>(e, 1.f, 1.f, 1.f);
    w.emplace<Velocity>(e, 2.f, 2.f, 2.f);
    w.emplace<Health>(e, 100);

    w.erase<Position>(e);
    w.erase<Velocity>(e);
    w.erase<Health>(e);

    EXPECT_FALSE(w.get<Position>(e).has_value());
    EXPECT_FALSE(w.get<Velocity>(e).has_value());
    EXPECT_FALSE(w.get<Health>(e).has_value());
}

// ============================================================================
// 10. In-place mutation via returned pointer
// ============================================================================

TEST(MutationViaPointer, directMutationPersists) {
    SmallWorld<Health, Position> w;
    auto e = w.create_entity();
    w.emplace<Health>(e, 100);

    w.get<Health>(e).value()->hp -= 30;
    EXPECT_EQ(w.get<Health>(e).value()->hp, 70);

    w.get<Health>(e).value()->hp -= 70;
    EXPECT_EQ(w.get<Health>(e).value()->hp, 0);
}

TEST(MutationViaPointer, mutatePosition_allAxes) {
    SmallWorld<Position, Velocity> w;
    auto e = w.create_entity();
    w.emplace<Position>(e, 0.f, 0.f, 0.f);

    auto *pos = w.get<Position>(e).value();
    pos->x = 100.f;
    pos->y = -50.f;
    pos->z = 3.14f;

    auto opt = w.get<Position>(e);
    ASSERT_TRUE(opt.has_value());
    EXPECT_FLOAT_EQ(opt.value()->x, 100.f);
    EXPECT_FLOAT_EQ(opt.value()->y, -50.f);
    EXPECT_FLOAT_EQ(opt.value()->z, 3.14f);
}

// ============================================================================
// 11. Stress / capacity growth
// ============================================================================

TEST(CapacityGrowth, manyEmplacesGrowStorage) {
    SmallWorld<Health> w;
    constexpr int N = 512;
    std::vector<Entity> entities;
    entities.reserve(N);

    for (int i = 0; i < N; ++i) {
        auto e = w.create_entity();
        w.emplace<Health>(e, i);
        entities.push_back(e);
    }

    // All values must still be correct after multiple reallocs
    for (int i = 0; i < N; ++i) {
        auto opt = w.get<Health>(entities[i]);
        ASSERT_TRUE(opt.has_value()) << "Entity " << i;
        EXPECT_EQ(opt.value()->hp, i) << "Entity " << i;
    }
}

TEST(CapacityGrowth, alternatingEmplaceAndErase) {
    SmallWorld<Health, Position> w;
    auto e = w.create_entity();

    for (int round = 0; round < 32; ++round) {
        w.emplace<Health>(e, round);
        ASSERT_TRUE(w.get<Health>(e).has_value());
        EXPECT_EQ(w.get<Health>(e).value()->hp, round);
        w.erase<Health>(e);
        EXPECT_FALSE(w.get<Health>(e).has_value());
    }
}

// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
