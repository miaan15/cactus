#include <cstddef>
#include <cstdint>

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

int main() {
    World world;

    constexpr int N = 50;

    // Phase 1: Create entities and emplace Position + Velocity on all of them
    Entity entities[N];
    for (int i = 0; i < N; ++i) {
        entities[i] = world.create_entity();

        auto res =
            world.emplace_component<Position>(entities[i], static_cast<float>(i),
                                              static_cast<float>(i * 2), static_cast<float>(i * 3));

        res = world.emplace_component<Velocity>(entities[i], static_cast<float>(i * 0.1f),
                                                static_cast<float>(i * 0.2f),
                                                static_cast<float>(i * 0.3f));
    }

    // Verify all entities have both components with correct values
    for (int i = 0; i < N; ++i) {

        auto pos = world.get_component<Position>(entities[i]);

        auto vel = world.get_component<Velocity>(entities[i]);
    }

    // Phase 2: Add Health to even-indexed entities
    for (int i = 0; i < N; i += 2) {
        auto res = world.emplace_component<Health>(entities[i], 100 + i);
    }

    // Verify Health presence/absence and that Position/Velocity are still correct
    for (int i = 0; i < N; ++i) {
        auto pos = world.get_component<Position>(entities[i]);

        auto vel = world.get_component<Velocity>(entities[i]);

        if (i % 2 == 0) {
            auto hp = world.get_component<Health>(entities[i]);
        } else {
        }
    }

    // Phase 3: Remove Velocity from odd-indexed entities
    for (int i = 1; i < N; i += 2) {
        auto res = world.remove_component<Velocity>(entities[i]);
    }

    // Verify final state
    for (int i = 0; i < N; ++i) {
        // Position should still be on everyone

        auto pos = world.get_component<Position>(entities[i]);

        if (i % 2 == 0) {
            // Even: has Velocity + Health

        } else {
            // Odd: Velocity removed, no Health
        }
    }

    // Phase 4: Overwrite Position on all entities (tests the "set" path)
    for (int i = 0; i < N; ++i) {
        auto res = world.emplace_component<Position>(entities[i], 999.0f, 888.0f, 777.0f);
    }

    for (int i = 0; i < N; ++i) {
        auto pos = world.get_component<Position>(entities[i]);
    }

    // Phase 5: Remove all components from a subset, leaving them as empty entities
    for (int i = 0; i < N; i += 5) {
        world.remove_component<Position>(entities[i]);
        if (world.contains_component<Velocity>(entities[i]))
            world.remove_component<Velocity>(entities[i]);
        if (world.contains_component<Health>(entities[i]))
            world.remove_component<Health>(entities[i]);
    }
}
