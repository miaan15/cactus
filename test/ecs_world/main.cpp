#include <cstdint>
#include <gtest/gtest.h>

import Ecs;

using namespace cactus::ecs;

struct Position {
    float x, y, z;
};
struct Velocity {
    float vx, vy, vz;
};
struct Health {
    int hp;
};

struct Tag {
    uint8_t value;
};

struct Transform {
    double mat[2];
    float scale;
    uint8_t flags;

    int32_t layer;
};
struct Rotation {
    float angle;
    double axis[2];
};

struct BigPayload {
    uint64_t data[8];
};

TEST(WorldStressTest, EmplaceAndRemoveManyComponent) {
    World world;

    constexpr int N = 50;

    // Phase 1: Create entities and emplace Position + Velocity on all of them
    Entity entities[N];
    for (int i = 0; i < N; ++i) {
        entities[i] = world.create_entity();
        ASSERT_TRUE(world.contains_entity(entities[i]));

        auto res =
            world.emplace_component<Position>(entities[i], static_cast<float>(i),
                                              static_cast<float>(i * 2), static_cast<float>(i * 3));

        res = world.emplace_component<Velocity>(entities[i], static_cast<float>(i * 0.1f),
                                                static_cast<float>(i * 0.2f),
                                                static_cast<float>(i * 0.3f));
    }

    // Verify all entities have both components with correct values
    for (int i = 0; i < N; ++i) {
        ASSERT_TRUE(world.contains_component<Position>(entities[i]));
        ASSERT_TRUE(world.contains_component<Velocity>(entities[i]));
        ASSERT_FALSE(world.contains_component<Health>(entities[i]));

        auto pos = world.get_component<Position>(entities[i]);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos.value()->x, static_cast<float>(i));
        EXPECT_FLOAT_EQ(pos.value()->y, static_cast<float>(i * 2));
        EXPECT_FLOAT_EQ(pos.value()->z, static_cast<float>(i * 3));

        auto vel = world.get_component<Velocity>(entities[i]);
        ASSERT_TRUE(vel.has_value());
        EXPECT_FLOAT_EQ(vel.value()->vx, static_cast<float>(i * 0.1f));
        EXPECT_FLOAT_EQ(vel.value()->vy, static_cast<float>(i * 0.2f));
        EXPECT_FLOAT_EQ(vel.value()->vz, static_cast<float>(i * 0.3f));
    }

    // Phase 2: Add Health to even-indexed entities
    for (int i = 0; i < N; i += 2) {
        auto res = world.emplace_component<Health>(entities[i], 100 + i);
    }

    // Verify Health presence/absence and that Position/Velocity are still correct
    for (int i = 0; i < N; ++i) {
        auto pos = world.get_component<Position>(entities[i]);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos.value()->x, static_cast<float>(i));

        auto vel = world.get_component<Velocity>(entities[i]);
        ASSERT_TRUE(vel.has_value());
        EXPECT_FLOAT_EQ(vel.value()->vx, static_cast<float>(i * 0.1f));

        if (i % 2 == 0) {
            ASSERT_TRUE(world.contains_component<Health>(entities[i]));
            auto hp = world.get_component<Health>(entities[i]);
            ASSERT_TRUE(hp.has_value());
            EXPECT_EQ(hp.value()->hp, 100 + i);
        } else {
            ASSERT_FALSE(world.contains_component<Health>(entities[i]));
        }
    }

    // Phase 3: Remove Velocity from odd-indexed entities
    for (int i = 1; i < N; i += 2) {
        auto res = world.remove_component<Velocity>(entities[i]);
        EXPECT_TRUE(res);
    }

    // Verify final state
    for (int i = 0; i < N; ++i) {
        // Position should still be on everyone
        ASSERT_TRUE(world.contains_component<Position>(entities[i]));
        auto pos = world.get_component<Position>(entities[i]);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos.value()->x, static_cast<float>(i));
        EXPECT_FLOAT_EQ(pos.value()->y, static_cast<float>(i * 2));
        EXPECT_FLOAT_EQ(pos.value()->z, static_cast<float>(i * 3));

        if (i % 2 == 0) {
            // Even: has Velocity + Health
            ASSERT_TRUE(world.contains_component<Velocity>(entities[i]));
            ASSERT_TRUE(world.contains_component<Health>(entities[i]));
        } else {
            // Odd: Velocity removed, no Health
            ASSERT_FALSE(world.contains_component<Velocity>(entities[i]));
            ASSERT_FALSE(world.contains_component<Health>(entities[i]));
        }
    }

    // Phase 4: Overwrite Position on all entities (tests the "set" path)
    for (int i = 0; i < N; ++i) {
        auto res = world.emplace_component<Position>(entities[i], 999.0f, 888.0f, 777.0f);
    }

    for (int i = 0; i < N; ++i) {
        auto pos = world.get_component<Position>(entities[i]);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos.value()->x, 999.0f);
        EXPECT_FLOAT_EQ(pos.value()->y, 888.0f);
        EXPECT_FLOAT_EQ(pos.value()->z, 777.0f);
    }

    // Phase 5: Remove all components from a subset, leaving them as empty entities
    for (int i = 0; i < N; i += 5) {
        world.remove_component<Position>(entities[i]);
        if (world.contains_component<Velocity>(entities[i]))
            world.remove_component<Velocity>(entities[i]);
        if (world.contains_component<Health>(entities[i]))
            world.remove_component<Health>(entities[i]);

        EXPECT_FALSE(world.contains_component<Position>(entities[i]));
        EXPECT_FALSE(world.contains_component<Velocity>(entities[i]));
        EXPECT_FALSE(world.contains_component<Health>(entities[i]));
        EXPECT_TRUE(world.contains_entity(entities[i]));
    }
}

TEST(WorldStressTest, EmplaceAndRemoveManyAndMANYComponent) {
    World world;

    constexpr int N = 200;

    Entity entities[N];
    for (int i = 0; i < N; ++i) {
        entities[i] = world.create_entity();
    }

    // Phase 1: Assign varied component combinations based on index modulo
    // This creates many different archetype signatures, stressing archetype migration.
    //
    // Pattern:
    //   All entities get Position.
    //   i % 2 == 0  -> also gets Velocity
    //   i % 3 == 0  -> also gets Health
    //   i % 5 == 0  -> also gets Tag
    //   i % 7 == 0  -> also gets Transform
    //   i % 11 == 0 -> also gets Rotation
    //   i % 13 == 0 -> also gets BigPayload
    for (int i = 0; i < N; ++i) {
        world.emplace_component<Position>(entities[i], static_cast<float>(i),
                                          static_cast<float>(-i), static_cast<float>(i + 1));

        if (i % 2 == 0)
            world.emplace_component<Velocity>(entities[i], static_cast<float>(i) * 0.5f, 0.0f,
                                              static_cast<float>(i) * -0.5f);

        if (i % 3 == 0) world.emplace_component<Health>(entities[i], i * 10);

        if (i % 5 == 0) world.emplace_component<Tag>(entities[i], static_cast<uint8_t>(i & 0xFF));

        if (i % 7 == 0)
            world.emplace_component<Transform>(
                entities[i], Transform{{static_cast<double>(i), static_cast<double>(i + 1)},
                                       static_cast<float>(i) * 0.1f,
                                       static_cast<uint8_t>(i % 4),
                                       static_cast<int32_t>(i)});

        if (i % 11 == 0)
            world.emplace_component<Rotation>(
                entities[i], Rotation{static_cast<float>(i) * 3.14f,
                                      {static_cast<double>(i), static_cast<double>(-i)}});

        if (i % 13 == 0) {
            BigPayload bp{};
            for (int k = 0; k < 8; ++k) bp.data[k] = static_cast<uint64_t>(i * 1000 + k);
            world.emplace_component<BigPayload>(entities[i], bp);
        }
    }

    // Phase 2: Verify everything is correct
    for (int i = 0; i < N; ++i) {
        ASSERT_TRUE(world.contains_entity(entities[i]));
        ASSERT_TRUE(world.contains_component<Position>(entities[i]));

        auto pos = world.get_component<Position>(entities[i]);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos.value()->x, static_cast<float>(i));
        EXPECT_FLOAT_EQ(pos.value()->y, static_cast<float>(-i));

        if (i % 2 == 0) {
            ASSERT_TRUE(world.contains_component<Velocity>(entities[i]));
            auto vel = world.get_component<Velocity>(entities[i]);
            ASSERT_TRUE(vel.has_value());
            EXPECT_FLOAT_EQ(vel.value()->vx, static_cast<float>(i) * 0.5f);
        } else {
            EXPECT_FALSE(world.contains_component<Velocity>(entities[i]));
        }

        if (i % 3 == 0) {
            ASSERT_TRUE(world.contains_component<Health>(entities[i]));
            auto hp = world.get_component<Health>(entities[i]);
            ASSERT_TRUE(hp.has_value());
            EXPECT_EQ(hp.value()->hp, i * 10);
        } else {
            EXPECT_FALSE(world.contains_component<Health>(entities[i]));
        }

        if (i % 5 == 0) {
            ASSERT_TRUE(world.contains_component<Tag>(entities[i]));
            auto tag = world.get_component<Tag>(entities[i]);
            ASSERT_TRUE(tag.has_value());
            EXPECT_EQ(tag.value()->value, static_cast<uint8_t>(i & 0xFF));
        }

        if (i % 7 == 0) {
            ASSERT_TRUE(world.contains_component<Transform>(entities[i]));
            auto tf = world.get_component<Transform>(entities[i]);
            ASSERT_TRUE(tf.has_value());
            EXPECT_DOUBLE_EQ(tf.value()->mat[0], static_cast<double>(i));
            EXPECT_DOUBLE_EQ(tf.value()->mat[1], static_cast<double>(i + 1));
            EXPECT_FLOAT_EQ(tf.value()->scale, static_cast<float>(i) * 0.1f);
            EXPECT_EQ(tf.value()->flags, static_cast<uint8_t>(i % 4));
            EXPECT_EQ(tf.value()->layer, static_cast<int32_t>(i));
        }

        if (i % 11 == 0) {
            ASSERT_TRUE(world.contains_component<Rotation>(entities[i]));
            auto rot = world.get_component<Rotation>(entities[i]);
            ASSERT_TRUE(rot.has_value());
            EXPECT_FLOAT_EQ(rot.value()->angle, static_cast<float>(i) * 3.14f);
            EXPECT_DOUBLE_EQ(rot.value()->axis[0], static_cast<double>(i));
            EXPECT_DOUBLE_EQ(rot.value()->axis[1], static_cast<double>(-i));
        }

        if (i % 13 == 0) {
            ASSERT_TRUE(world.contains_component<BigPayload>(entities[i]));
            auto bp = world.get_component<BigPayload>(entities[i]);
            ASSERT_TRUE(bp.has_value());
            for (int k = 0; k < 8; ++k)
                EXPECT_EQ(bp.value()->data[k], static_cast<uint64_t>(i * 1000 + k));
        }
    }

    // Phase 3: Bulk remove — remove Velocity from everyone who has it,
    // then verify remaining components survived migration intact.
    for (int i = 0; i < N; i += 2) {
        auto res = world.remove_component<Velocity>(entities[i]);
        EXPECT_TRUE(res);
    }

    for (int i = 0; i < N; ++i) {
        EXPECT_FALSE(world.contains_component<Velocity>(entities[i]));

        // Position must still be intact
        auto pos = world.get_component<Position>(entities[i]);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos.value()->x, static_cast<float>(i));

        // Health must still be intact for i % 3 == 0
        if (i % 3 == 0) {
            auto hp = world.get_component<Health>(entities[i]);
            ASSERT_TRUE(hp.has_value());
            EXPECT_EQ(hp.value()->hp, i * 10);
        }

        // BigPayload must still be intact for i % 13 == 0
        if (i % 13 == 0) {
            auto bp = world.get_component<BigPayload>(entities[i]);
            ASSERT_TRUE(bp.has_value());
            for (int k = 0; k < 8; ++k)
                EXPECT_EQ(bp.value()->data[k], static_cast<uint64_t>(i * 1000 + k));
        }
    }

    // Phase 4: Re-add Velocity to a subset and add a brand-new component (Tag) to others.
    // This tests re-migration into archetypes that already exist.
    for (int i = 0; i < N; i += 4) {
        world.emplace_component<Velocity>(entities[i], 1.0f, 2.0f, 3.0f);
    }
    for (int i = 1; i < N; i += 4) {
        world.emplace_component<Tag>(entities[i], static_cast<uint8_t>(42));
    }

    for (int i = 0; i < N; i += 4) {
        ASSERT_TRUE(world.contains_component<Velocity>(entities[i]));
        auto vel = world.get_component<Velocity>(entities[i]);
        ASSERT_TRUE(vel.has_value());
        EXPECT_FLOAT_EQ(vel.value()->vx, 1.0f);
        EXPECT_FLOAT_EQ(vel.value()->vy, 2.0f);
        EXPECT_FLOAT_EQ(vel.value()->vz, 3.0f);

        // Position still intact after re-migration
        auto pos = world.get_component<Position>(entities[i]);
        ASSERT_TRUE(pos.has_value());
        EXPECT_FLOAT_EQ(pos.value()->x, static_cast<float>(i));
    }

    for (int i = 1; i < N; i += 4) {
        ASSERT_TRUE(world.contains_component<Tag>(entities[i]));
        auto tag = world.get_component<Tag>(entities[i]);
        ASSERT_TRUE(tag.has_value());
        EXPECT_EQ(tag.value()->value, 42);
    }

    // Phase 5: Mass removal — strip every component off every 10th entity
    for (int i = 0; i < N; i += 10) {
        if (world.contains_component<Position>(entities[i]))
            world.remove_component<Position>(entities[i]);
        if (world.contains_component<Velocity>(entities[i]))
            world.remove_component<Velocity>(entities[i]);
        if (world.contains_component<Health>(entities[i]))
            world.remove_component<Health>(entities[i]);
        if (world.contains_component<Tag>(entities[i])) world.remove_component<Tag>(entities[i]);
        if (world.contains_component<Transform>(entities[i]))
            world.remove_component<Transform>(entities[i]);
        if (world.contains_component<Rotation>(entities[i]))
            world.remove_component<Rotation>(entities[i]);
        if (world.contains_component<BigPayload>(entities[i]))
            world.remove_component<BigPayload>(entities[i]);

        // Entity still exists but has no components
        EXPECT_TRUE(world.contains_entity(entities[i]));
        EXPECT_FALSE(world.contains_component<Position>(entities[i]));
        EXPECT_FALSE(world.contains_component<Velocity>(entities[i]));
        EXPECT_FALSE(world.contains_component<Health>(entities[i]));
        EXPECT_FALSE(world.contains_component<Tag>(entities[i]));
        EXPECT_FALSE(world.contains_component<Transform>(entities[i]));
        EXPECT_FALSE(world.contains_component<Rotation>(entities[i]));
        EXPECT_FALSE(world.contains_component<BigPayload>(entities[i]));
    }

    // Final sanity: non-stripped entities still have Position
    for (int i = 0; i < N; ++i) {
        if (i % 10 != 0) {
            ASSERT_TRUE(world.contains_component<Position>(entities[i]))
                << "Entity " << i << " lost Position unexpectedly";
            auto pos = world.get_component<Position>(entities[i]);
            ASSERT_TRUE(pos.has_value());
            EXPECT_FLOAT_EQ(pos.value()->x, static_cast<float>(i));
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
