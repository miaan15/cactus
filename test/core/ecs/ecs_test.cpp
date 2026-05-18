#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cassert>
#include <doctest.h>

import std;
import cactus;

using namespace cactus;

struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};

struct Health {
    int value;
};

TEST_CASE("EcsWorld_EntityAndSignature") {
    auto world = World<Position, Velocity, Health>::make();

    Entity e1 = world.new_entity();
    Entity e2 = world.new_entity();

    CHECK(world.has_entity(e1));
    CHECK(world.has_entity(e2));

    auto sig1 = world.get_entity_signature(e1);
    REQUIRE(sig1.has_value());
    CHECK(sig1->none());

    Position *pos = world.add_component<Position>(e1);
    pos->x = 1.0f;
    pos->y = 2.0f;
    auto sig_after = world.get_entity_signature(e1);
    REQUIRE(sig_after.has_value());
    CHECK(sig_after->count() == 1);
    CHECK(world.has_component<Position>(e1));
    CHECK_FALSE(world.has_component<Velocity>(e1));

    world.add_component<Velocity>(e1);
    CHECK(world.get_entity_signature(e1)->count() == 2);

    world.destroy();
}

TEST_CASE("EcsWorld_AddRemoveGetComponent") {
    auto world = World<Position, Velocity, Health>::make();

    Entity e = world.new_entity();

    SUBCASE("Add single component") {
        Position *pos = world.add_component<Position>(e);
        REQUIRE(pos != nullptr);
        pos->x = 10.0f;
        pos->y = 20.0f;
        CHECK(pos->x == 10.0f);
        CHECK(pos->y == 20.0f);
    }

    SUBCASE("Get component by value") {
        Health *health = world.add_component<Health>(e);
        health->value = 100;
        auto health_val = world.get_component<Health>(e);
        REQUIRE(health_val.has_value());
        CHECK(health_val->value == 100);
    }

    SUBCASE("Get component by pointer") {
        Velocity *vel = world.add_component<Velocity>(e);
        vel->dx = 1.5f;
        vel->dy = 2.5f;
        Velocity *vel_ptr = world.get_component_ptr<Velocity>(e);
        REQUIRE(vel_ptr != nullptr);
        CHECK(vel_ptr->dx == 1.5f);
        CHECK(vel_ptr->dy == 2.5f);
    }

    SUBCASE("Add multiple components") {
        Position *pos = world.add_component<Position>(e);
        pos->x = 1.0f;
        pos->y = 2.0f;
        world.add_component<Velocity>(e);
        world.add_component<Health>(e);
        CHECK(world.has_component<Position>(e));
        CHECK(world.has_component<Velocity>(e));
        CHECK(world.has_component<Health>(e));
    }

    SUBCASE("Remove component") {
        Position *pos = world.add_component<Position>(e);
        pos->x = 5.0f;
        pos->y = 10.0f;
        Health *health = world.add_component<Health>(e);
        health->value = 50;
        CHECK(world.has_component<Position>(e));

        bool removed = world.remove_component<Position>(e);
        CHECK(removed);
        CHECK_FALSE(world.has_component<Position>(e));
        CHECK(world.has_component<Health>(e));
    }

    SUBCASE("Get non-existent component") {
        auto pos = world.get_component<Position>(e);
        CHECK_FALSE(pos.has_value());

        Position *pos_ptr = world.get_component_ptr<Position>(e);
        CHECK(pos_ptr == nullptr);
    }

    world.destroy();
}

TEST_CASE("EcsWorld_StressTest") {
    auto world = World<Position, Velocity, Health>::make();

    Entity entities[32];
    for (int i = 0; i < 32; i++) { entities[i] = world.new_entity(); }

    CHECK(world.world_impl.entities_data.len == 32);

    SUBCASE("Add components to all entities") {
        for (int i = 0; i < 32; i++) {
            Position *pos = world.add_component<Position>(entities[i]);
            pos->x = static_cast<float>(i);
            pos->y = static_cast<float>(i * 2);
        }
        for (int i = 0; i < 32; i++) {
            CHECK(world.has_component<Position>(entities[i]));
            auto pos = world.get_component<Position>(entities[i]);
            REQUIRE(pos.has_value());
            CHECK(pos->x == static_cast<float>(i));
        }

        CHECK(world.world_impl.signature_to_table_index_map.size() == 1);
        CHECK(world.world_impl.tables.size() == 1);
        CHECK(world.world_impl.tables[0].len == 32);
    }

    SUBCASE("Repeated add and remove") {
        Entity e = entities[10];
        for (int i = 0; i < 10; i++) {
            CHECK_FALSE(world.has_component<Position>(e));
            Position *pos = world.add_component<Position>(e);
            pos->x = 1.0f;
            pos->y = 2.0f;
            CHECK(world.has_component<Position>(e));
            world.remove_component<Position>(e);
            CHECK(world.get_component_ptr<Position>(e) == nullptr);
        }
        CHECK_FALSE(world.has_component<Position>(e));

        CHECK(world.world_impl.tables[0].len == 0);
    }

    SUBCASE("Add duplicate component") {
        Entity e = entities[20];
        Position *p1 = world.add_component<Position>(e);
        p1->x = 10.0f;
        p1->y = 20.0f;
        Position *p2 = world.add_component<Position>(e);
        CHECK(p1 == p2);
        CHECK(p2->x == 10.0f);
        CHECK(p2->y == 20.0f);

        CHECK(world.world_impl.tables[0].len == 1);
    }

    SUBCASE("Remove non-existent component") {
        Entity e = entities[3];
        bool removed = world.remove_component<Position>(e);
        CHECK_FALSE(removed);
    }

    SUBCASE("Mixed components per entity") {
        Position *pos0 = world.add_component<Position>(entities[0]);
        pos0->x = 1.0f;
        pos0->y = 2.0f;
        Velocity *vel0 = world.add_component<Velocity>(entities[0]);
        vel0->dx = 0.5f;
        vel0->dy = 0.5f;

        CHECK(world.world_impl.signature_to_table_index_map.size() == 2);
        CHECK(world.world_impl.tables.size() == 2);
        CHECK(world.world_impl.tables[0].len == 0); // entities[0] migrated away
        CHECK(world.world_impl.tables[1].len == 1); // holds entities[0] ([Pos, Vel])

        // Variadic bulk allocation bypasses intermediate table footprints
        world.add_component<Position, Health, Velocity>(entities[1]);

        CHECK(world.world_impl.signature_to_table_index_map.size() == 3);
        CHECK(world.world_impl.tables.size() == 3);
        CHECK(world.world_impl.tables[0].len == 0); // [Pos]
        CHECK(world.world_impl.tables[1].len == 1); // [Pos, Vel] -> entities[0]
        CHECK(world.world_impl.tables[2].len == 1); // [Pos, Vel, Health] -> entities[1]

        world.remove_component<Health>(entities[1]);

        // Remap targets existing archetype [Pos, Vel]
        CHECK(world.world_impl.signature_to_table_index_map.size() == 3);
        CHECK(world.world_impl.tables.size() == 3);
        CHECK(world.world_impl.tables[0].len == 0); // [Pos]
        CHECK(world.world_impl.tables[1].len == 2); // [Pos, Vel] -> entities[0], entities[1]
        CHECK(world.world_impl.tables[2].len == 0); // [Pos, Vel, Health] is now empty

        world.add_component<Health>(entities[1]); // entities[1] moves back to [Pos, Vel, Health]

        world.add_component<Health>(entities[2]); // Creates a unique [Health] table

        CHECK(world.world_impl.signature_to_table_index_map.size() == 4);
        CHECK(world.world_impl.tables.size() == 4);
        CHECK(world.world_impl.tables[0].len == 0); // [Pos]
        CHECK(world.world_impl.tables[1].len == 1); // [Pos, Vel] -> entities[0]
        CHECK(world.world_impl.tables[2].len == 1); // [Pos, Vel, Health] -> entities[1]
        CHECK(world.world_impl.tables[3].len == 1); // [Health] -> entities[2]

        CHECK(world.has_component<Position>(entities[0]));
        CHECK(world.has_component<Velocity>(entities[0]));
        CHECK_FALSE(world.has_component<Health>(entities[0]));

        CHECK(world.has_component<Position>(entities[1]));
        CHECK(world.has_component<Velocity>(entities[1]));
        CHECK(world.has_component<Health>(entities[1]));

        CHECK_FALSE(world.has_component<Position>(entities[2]));
        CHECK_FALSE(world.has_component<Velocity>(entities[2]));
        CHECK(world.has_component<Health>(entities[2]));
    }

    SUBCASE("Component migration between archetypes") {
        Entity e = entities[0];
        Position *pos = world.add_component<Position>(e);
        pos->x = 100.0f;
        pos->y = 200.0f;
        REQUIRE(pos != nullptr);

        for (int i = 0; i < 5; i++) {
            world.add_component<Velocity>(e);
            CHECK(world.has_component<Position>(e));
            CHECK(world.has_component<Velocity>(e));

            auto current_pos = world.get_component<Position>(e);
            REQUIRE(current_pos.has_value());
            CHECK(current_pos->x == 100.0f);
            CHECK(current_pos->y == 200.0f);

            world.remove_component<Velocity>(e);
            CHECK(world.has_component<Position>(e));
            CHECK_FALSE(world.has_component<Velocity>(e));
        }

        CHECK(world.world_impl.signature_to_table_index_map.size() == 2);
        CHECK(world.world_impl.tables.size() == 2);
        CHECK(world.world_impl.tables[0].len == 1); // [Pos] contains entity 'e'
        CHECK(world.world_impl.tables[1].len == 0); // [Pos, Vel] is empty
    }

    SUBCASE("Remove all components") {
        Entity e = entities[0];
        world.add_component<Position>(e);
        world.add_component<Velocity>(e);
        world.add_component<Health>(e);

        CHECK(world.get_entity_signature(e)->count() == 3);

        world.remove_component<Position>(e);
        world.remove_component<Velocity>(e);
        world.remove_component<Health>(e);
        CHECK(world.get_entity_signature(e)->none());
    }

    SUBCASE("Turbulent randomized structural stress") {
        // Enforce deterministic seed sequence to secure reproducibility across test passes
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> entity_dist(0, 31);
        std::uniform_int_distribution<int> action_dist(0, 1);
        std::uniform_int_distribution<int> comp_dist(0, 2);

        // Map abstract selection space to your register compilation tokens
        constexpr size_t POS_IDX = 0;
        constexpr size_t VEL_IDX = 1;
        constexpr size_t HLT_IDX = 2;
        const size_t comp_mapping[3] = {POS_IDX, VEL_IDX, HLT_IDX};

        // Hammer the world with structural changes to verify stability of the swap-and-pop logic
        for (int i = 0; i < 1000; i++) {
            Entity target_entity = entities[entity_dist(rng)];
            int action = action_dist(rng);
            size_t component_idx = comp_mapping[comp_dist(rng)];

            if (action == 0) { // Mutate via component inclusion
                if (component_idx == POS_IDX)
                    world.add_component<Position>(target_entity);
                else if (component_idx == VEL_IDX)
                    world.add_component<Velocity>(target_entity);
                else if (component_idx == HLT_IDX)
                    world.add_component<Health>(target_entity);
            } else { // Mutate via component exclusion
                if (component_idx == POS_IDX)
                    world.remove_component<Position>(target_entity);
                else if (component_idx == VEL_IDX)
                    world.remove_component<Velocity>(target_entity);
                else if (component_idx == HLT_IDX)
                    world.remove_component<Health>(target_entity);
            }
        }

        // Validate the tracking layers are completely in sync after the chaos
        size_t aggregate_table_len = 0;
        for (const auto &table : world.world_impl.tables) {
            aggregate_table_len += table.len;

            // Make sure internal tracking mirrors physical owner slots
            for (size_t row = 0; row < table.len; ++row) {
                Entity internal_owner = table.owner_list_raw[row];
                REQUIRE(world.has_entity(internal_owner));

                auto stored_data = world.world_impl.entities_data.get(internal_owner).value();
                CHECK(stored_data.table_row_index == row);
            }
        }

        // Every active entity must be tracked exactly once across the active archetypes
        size_t active_entities = 0;
        for (int i = 0; i < 32; i++) {
            auto sig = world.get_entity_signature(entities[i]);
            if (sig && sig->any()) { active_entities++; }
        }
        CHECK(aggregate_table_len == active_entities);
    }

    world.destroy();
}

TEST_CASE("EcsWorld_Query") {
    auto world = World<Position, Velocity, Health>::make();

    Entity entities[10];
    for (int i = 0; i < 10; i++) { entities[i] = world.new_entity(); }

    // Setup: entities[0-3] have Position, entities[4-6] have Position+Velocity, entities[7-9] have all three
    for (int i = 0; i < 10; i++) {
        Position *pos = world.add_component<Position>(entities[i]);
        pos->x = static_cast<float>(i);
        pos->y = static_cast<float>(i * 2);

        if (i >= 4) {
            Velocity *vel = world.add_component<Velocity>(entities[i]);
            vel->dx = 0.5f + i * 0.1f;
            vel->dy = 1.0f + i * 0.2f;
        }
        if (i >= 7) {
            Health *health = world.add_component<Health>(entities[i]);
            health->value = 100 - i * 5;
        }
    }

    SUBCASE("Query with Position only") {
        auto query = world.query_builder().with<Position>().build();
        int count = 0;
        float x_sum = 0.0f;
        for (auto [entity, prefab] : query) {
            auto pos = prefab.get<Position>();
            REQUIRE(pos.has_value());
            x_sum += pos->x;
            count++;
        }
        CHECK(count == 10);
        CHECK(x_sum == doctest::Approx(45.0f)); // 0+1+...+9
    }

    SUBCASE("Query with Position and Velocity") {
        auto query = world.query_builder().with<Position, Velocity>().build();
        int count = 0;
        for (auto [entity, prefab] : query) {
            CHECK(prefab.get_ptr<Position>() != nullptr);
            CHECK(prefab.get_ptr<Velocity>() != nullptr);
            count++;
        }
        CHECK(count == 6); // entities[4-9]
    }

    SUBCASE("Query with all components") {
        auto query = world.query_builder().with<Position, Velocity, Health>().build();
        int count = 0;
        for (auto [entity, prefab] : query) {
            auto pos = prefab.get<Position>();
            auto vel = prefab.get<Velocity>();
            auto hp = prefab.get<Health>();
            REQUIRE(pos.has_value());
            REQUIRE(vel.has_value());
            REQUIRE(hp.has_value());
            CHECK(pos->x + vel->dx + hp->value > 0);
            count++;
        }
        CHECK(count == 3); // entities[7-9]
    }

    SUBCASE("Query non-existent combination") {
        auto query = world.query_builder().with<Health>().build();
        int count = 0;
        for (auto [entity, prefab] : query) { count++; }
        CHECK(count == 3); // entities[7-9]
    }

    world.destroy();
}
