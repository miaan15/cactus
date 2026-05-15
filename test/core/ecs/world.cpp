#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

import cactus;

using namespace cactus;

struct Position {
    float x;
    float y;
};

struct Velocity {
    float dx;
    float dy;
};

struct Health {
    int hp;
    int max_hp;
};

TEST_CASE("World: CreateHasDestroyAddRemoveGet") {
    World world = World::make();

    Entity e = world.create();
    REQUIRE(world.has(e));

    REQUIRE_FALSE(world.has_component<Position>(e));
    REQUIRE_FALSE(world.has_component<Velocity>(e));

    Position pos{.x = 10.0f, .y = 20.0f};
    REQUIRE(world.add_component(e, pos));

    REQUIRE(world.has_component<Position>(e));
    auto pos_opt = world.get_component<Position>(e);
    REQUIRE(pos_opt.has_value());

    // Use doctest::Approx for floating point comparisons
    CHECK(pos_opt->x == doctest::Approx(10.0f));
    CHECK(pos_opt->y == doctest::Approx(20.0f));

    pos.x = 15.0f;
    REQUIRE(world.set_component(e, pos));
    auto pos_opt2 = world.get_component<Position>(e);
    CHECK(pos_opt2->x == doctest::Approx(15.0f));

    REQUIRE(world.remove_component<Position>(e));
    REQUIRE_FALSE(world.has_component<Position>(e));

    world.destroy();
}

TEST_CASE("WorldView: IterateEntitiesWithComponents") {
    World world = World::make();

    Entity e1 = world.create();
    world.add_component(e1, Position{.x = 1.0f, .y = 2.0f});
    world.add_component(e1, Velocity{.dx = 0.5f, .dy = 1.0f});

    Entity e2 = world.create();
    world.add_component(e2, Position{.x = 5.0f, .y = 10.0f});
    world.add_component(e2, Velocity{.dx = -0.5f, .dy = -1.0f});

    Entity e3 = world.create();
    world.add_component(e3, Position{.x = 100.0f, .y = 200.0f});

    WorldView<Position, Velocity> view(world);

    int count = 0;
    for (auto [entity, pos, vel] : view) {
        REQUIRE(world.has(entity));
        REQUIRE(world.has_component<Position>(entity));
        REQUIRE(world.has_component<Velocity>(entity));
        pos->x += vel->dx;
        pos->y += vel->dy;
        ++count;
    }
    CHECK(count == 2);

    auto e1_pos = world.get_component<Position>(e1);
    CHECK(e1_pos->x == doctest::Approx(1.5f));
    CHECK(e1_pos->y == doctest::Approx(3.0f));

    world.destroy();
}

TEST_CASE("WorldIntegration: Multiple Entities Component Mutation") {
    World world = World::make();

    Entity player = world.create();
    world.add_component(player, Position{.x = 0.0f, .y = 0.0f});
    world.add_component(player, Velocity{.dx = 1.0f, .dy = 0.5f});
    world.add_component(player, Health{.hp = 100, .max_hp = 100});

    Entity enemy = world.create();
    world.add_component(enemy, Position{.x = 50.0f, .y = 50.0f});
    world.add_component(enemy, Health{.hp = 50, .max_hp = 50});

    Entity wall = world.create();
    world.add_component(wall, Position{.x = 25.0f, .y = 25.0f});

    {
        WorldView<Position, Velocity> moving_view(world);
        for (auto [entity, pos, vel] : moving_view) {
            pos->x += vel->dx;
            pos->y += vel->dy;
        }
    }

    {
        WorldView<Health> health_view(world);
        for (auto [entity, hp] : health_view) { hp->hp -= 10; }
    }

    auto player_pos = world.get_component<Position>(player);
    CHECK(player_pos->x == doctest::Approx(1.0f));

    auto player_hp = world.get_component<Health>(player);
    CHECK(player_hp->hp == 90);

    auto enemy_hp = world.get_component<Health>(enemy);
    CHECK(enemy_hp->hp == 40);

    REQUIRE_FALSE(world.has_component<Health>(wall));

    world.remove_component<Velocity>(player);
    REQUIRE_FALSE(world.has_component<Velocity>(player));
    REQUIRE(world.has_component<Position>(player));

    world.destroy();
}
