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

struct Marker {};
struct Frozen {};
struct IsPlayer {};

struct alignas(64) OddAlignment {
    uint8_t a;  // offset 0
    uint64_t b; // offset 8  (7 bytes padding after a)
    uint8_t c;  // offset 16
    uint32_t d; // offset 20 (3 bytes padding after c)
    uint8_t e;  // offset 24
    // sizeof likely 32 with tail padding
};

struct PackedMix {
    char tag;     // 1 byte
    double value; // 8 bytes (7 bytes padding after tag)
    bool flag;    // 1 byte
    int16_t idx;  // 2 bytes (1 byte padding after flag)
    // sizeof likely 24
};

TEST(WorldStressTest, EmplaceAndRemoveComponentManyTimes) {
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
    //   i % 4 == 0  -> also gets Marker       (zero-sized tag)
    //   i % 9 == 0  -> also gets Frozen        (zero-sized tag)
    //   i % 6 == 0  -> also gets IsPlayer      (zero-sized tag)
    //   i % 8 == 0  -> also gets OddAlignment  (off-alignment)
    //   i % 10 == 0 -> also gets PackedMix     (off-alignment)
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

        // Zero-sized tags
        if (i % 4 == 0) world.emplace_component<Marker>(entities[i]);

        if (i % 9 == 0) world.emplace_component<Frozen>(entities[i]);

        if (i % 6 == 0) world.emplace_component<IsPlayer>(entities[i]);

        // Off-alignment components
        if (i % 8 == 0)
            world.emplace_component<OddAlignment>(
                entities[i],
                OddAlignment{static_cast<uint8_t>(i & 0xFF), static_cast<uint64_t>(i) * 7,
                             static_cast<uint8_t>((i + 1) & 0xFF), static_cast<uint32_t>(i * 3),
                             static_cast<uint8_t>((i + 2) & 0xFF)});

        if (i % 10 == 0)
            world.emplace_component<PackedMix>(entities[i],
                                               PackedMix{static_cast<char>('A' + (i % 26)),
                                                         static_cast<double>(i) * 1.111,
                                                         (i % 2 == 0), static_cast<int16_t>(i)});
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

        // Verify zero-sized tags
        if (i % 4 == 0) {
            ASSERT_TRUE(world.contains_component<Marker>(entities[i]))
                << "Entity " << i << " should have Marker";
        } else {
            EXPECT_FALSE(world.contains_component<Marker>(entities[i]));
        }

        if (i % 9 == 0) {
            ASSERT_TRUE(world.contains_component<Frozen>(entities[i]))
                << "Entity " << i << " should have Frozen";
        } else {
            EXPECT_FALSE(world.contains_component<Frozen>(entities[i]));
        }

        if (i % 6 == 0) {
            ASSERT_TRUE(world.contains_component<IsPlayer>(entities[i]))
                << "Entity " << i << " should have IsPlayer";
        } else {
            EXPECT_FALSE(world.contains_component<IsPlayer>(entities[i]));
        }

        // Verify off-alignment components
        if (i % 8 == 0) {
            ASSERT_TRUE(world.contains_component<OddAlignment>(entities[i]));
            auto oa = world.get_component<OddAlignment>(entities[i]);
            ASSERT_TRUE(oa.has_value());
            EXPECT_EQ(oa.value()->a, static_cast<uint8_t>(i & 0xFF));
            EXPECT_EQ(oa.value()->b, static_cast<uint64_t>(i) * 7);
            EXPECT_EQ(oa.value()->c, static_cast<uint8_t>((i + 1) & 0xFF));
            EXPECT_EQ(oa.value()->d, static_cast<uint32_t>(i * 3));
            EXPECT_EQ(oa.value()->e, static_cast<uint8_t>((i + 2) & 0xFF));
        } else {
            EXPECT_FALSE(world.contains_component<OddAlignment>(entities[i]));
        }

        if (i % 10 == 0) {
            ASSERT_TRUE(world.contains_component<PackedMix>(entities[i]));
            auto pm = world.get_component<PackedMix>(entities[i]);
            ASSERT_TRUE(pm.has_value());
            EXPECT_EQ(pm.value()->tag, static_cast<char>('A' + (i % 26)));
            EXPECT_DOUBLE_EQ(pm.value()->value, static_cast<double>(i) * 1.111);
            EXPECT_EQ(pm.value()->flag, (i % 2 == 0));
            EXPECT_EQ(pm.value()->idx, static_cast<int16_t>(i));
        } else {
            EXPECT_FALSE(world.contains_component<PackedMix>(entities[i]));
        }
    }

    // Phase 3: Bulk remove — remove Velocity from everyone who has it,
    // then verify remaining components survived migration intact.
    for (int i = 0; i < N; i += 2) {
        auto res = world.remove_component<Velocity>(entities[i]);
        EXPECT_TRUE(res);
    }

    // Also remove Marker from everyone who has it — stress zero-sized removal
    for (int i = 0; i < N; i += 4) {
        auto res = world.remove_component<Marker>(entities[i]);
        EXPECT_TRUE(res);
    }

    // Remove OddAlignment from every 16th entity (subset of those who have it)
    for (int i = 0; i < N; i += 16) {
        if (world.contains_component<OddAlignment>(entities[i])) {
            auto res = world.remove_component<OddAlignment>(entities[i]);
            EXPECT_TRUE(res);
        }
    }

    for (int i = 0; i < N; ++i) {
        EXPECT_FALSE(world.contains_component<Velocity>(entities[i]));
        EXPECT_FALSE(world.contains_component<Marker>(entities[i]));

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

        // Frozen tag must still be intact for i % 9 == 0
        if (i % 9 == 0) {
            ASSERT_TRUE(world.contains_component<Frozen>(entities[i]))
                << "Entity " << i << " lost Frozen after Velocity/Marker removal";
        }

        // IsPlayer tag must still be intact for i % 6 == 0
        if (i % 6 == 0) {
            ASSERT_TRUE(world.contains_component<IsPlayer>(entities[i]))
                << "Entity " << i << " lost IsPlayer after Velocity/Marker removal";
        }

        // OddAlignment: removed from i%16==0, still on i%8==0 && i%16!=0
        if (i % 8 == 0 && i % 16 != 0) {
            ASSERT_TRUE(world.contains_component<OddAlignment>(entities[i]));
            auto oa = world.get_component<OddAlignment>(entities[i]);
            ASSERT_TRUE(oa.has_value());
            EXPECT_EQ(oa.value()->b, static_cast<uint64_t>(i) * 7);
        } else if (i % 16 == 0) {
            EXPECT_FALSE(world.contains_component<OddAlignment>(entities[i]));
        }

        // PackedMix must still be intact for i % 10 == 0
        if (i % 10 == 0) {
            ASSERT_TRUE(world.contains_component<PackedMix>(entities[i]));
            auto pm = world.get_component<PackedMix>(entities[i]);
            ASSERT_TRUE(pm.has_value());
            EXPECT_DOUBLE_EQ(pm.value()->value, static_cast<double>(i) * 1.111);
        }
    }

    // Phase 4: Re-add Velocity to a subset and add a brand-new component (Tag) to others.
    // This tests re-migration into archetypes that already exist.
    // Also re-add Marker (zero-sized) and OddAlignment (off-alignment) to new subsets.
    for (int i = 0; i < N; i += 4) {
        world.emplace_component<Velocity>(entities[i], 1.0f, 2.0f, 3.0f);
    }
    for (int i = 1; i < N; i += 4) {
        world.emplace_component<Tag>(entities[i], static_cast<uint8_t>(42));
    }

    // Re-add Marker to every 3rd entity — different pattern than original
    for (int i = 0; i < N; i += 3) {
        world.emplace_component<Marker>(entities[i]);
    }

    // Re-add OddAlignment to every 16th entity that had it removed
    for (int i = 0; i < N; i += 16) {
        world.emplace_component<OddAlignment>(
            entities[i], OddAlignment{static_cast<uint8_t>(0xAA), static_cast<uint64_t>(i) * 77,
                                      static_cast<uint8_t>(0xBB), static_cast<uint32_t>(i + 99),
                                      static_cast<uint8_t>(0xCC)});
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

    // Verify re-added Marker
    for (int i = 0; i < N; i += 3) {
        ASSERT_TRUE(world.contains_component<Marker>(entities[i]))
            << "Entity " << i << " should have re-added Marker";
    }

    // Verify re-added OddAlignment at i%16==0
    for (int i = 0; i < N; i += 16) {
        ASSERT_TRUE(world.contains_component<OddAlignment>(entities[i]));
        auto oa = world.get_component<OddAlignment>(entities[i]);
        ASSERT_TRUE(oa.has_value());
        EXPECT_EQ(oa.value()->a, static_cast<uint8_t>(0xAA));
        EXPECT_EQ(oa.value()->b, static_cast<uint64_t>(i) * 77);
        EXPECT_EQ(oa.value()->c, static_cast<uint8_t>(0xBB));
        EXPECT_EQ(oa.value()->d, static_cast<uint32_t>(i + 99));
        EXPECT_EQ(oa.value()->e, static_cast<uint8_t>(0xCC));
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
        if (world.contains_component<Marker>(entities[i]))
            world.remove_component<Marker>(entities[i]);
        if (world.contains_component<Frozen>(entities[i]))
            world.remove_component<Frozen>(entities[i]);
        if (world.contains_component<IsPlayer>(entities[i]))
            world.remove_component<IsPlayer>(entities[i]);
        if (world.contains_component<OddAlignment>(entities[i]))
            world.remove_component<OddAlignment>(entities[i]);
        if (world.contains_component<PackedMix>(entities[i]))
            world.remove_component<PackedMix>(entities[i]);

        // Entity still exists but has no components
        EXPECT_TRUE(world.contains_entity(entities[i]));
        EXPECT_FALSE(world.contains_component<Position>(entities[i]));
        EXPECT_FALSE(world.contains_component<Velocity>(entities[i]));
        EXPECT_FALSE(world.contains_component<Health>(entities[i]));
        EXPECT_FALSE(world.contains_component<Tag>(entities[i]));
        EXPECT_FALSE(world.contains_component<Transform>(entities[i]));
        EXPECT_FALSE(world.contains_component<Rotation>(entities[i]));
        EXPECT_FALSE(world.contains_component<BigPayload>(entities[i]));
        EXPECT_FALSE(world.contains_component<Marker>(entities[i]));
        EXPECT_FALSE(world.contains_component<Frozen>(entities[i]));
        EXPECT_FALSE(world.contains_component<IsPlayer>(entities[i]));
        EXPECT_FALSE(world.contains_component<OddAlignment>(entities[i]));
        EXPECT_FALSE(world.contains_component<PackedMix>(entities[i]));
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
