#include <gtest/gtest.h>

import gtest;

import cactus.core.ecs;

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

TEST(WorldTest, CreateHasDestroyAddRemoveGet) {
    World world = World::make();

    Entity e = world.create();
    ASSERT_TRUE(world.has(e));

    ASSERT_FALSE(world.has_component<Position>(e));
    ASSERT_FALSE(world.has_component<Velocity>(e));

    Position pos{.x = 10.0f, .y = 20.0f};
    ASSERT_TRUE(world.add_component(e, pos));

    ASSERT_TRUE(world.has_component<Position>(e));
    auto pos_opt = world.get_component<Position>(e);
    ASSERT_TRUE(pos_opt.has_value());
    EXPECT_FLOAT_EQ(pos_opt->x, 10.0f);
    EXPECT_FLOAT_EQ(pos_opt->y, 20.0f);

    pos.x = 15.0f;
    ASSERT_TRUE(world.set_component(e, pos));
    auto pos_opt2 = world.get_component<Position>(e);
    EXPECT_FLOAT_EQ(pos_opt2->x, 15.0f);

    ASSERT_TRUE(world.remove_component<Position>(e));
    ASSERT_FALSE(world.has_component<Position>(e));

    world.destroy();
}

TEST(WorldViewTest, IterateEntitiesWithComponents) {
    World world = World::make();

    Entity e1 = world.create();
    world.add_component(e1, Position{.x = 1.0f, .y = 2.0f});
    world.add_component(e1, Velocity{.dx = 0.5f, .dy = 1.0f});

    Entity e2 = world.create();
    world.add_component(e2, Position{.x = 5.0f, .y = 10.0f});
    world.add_component(e2, Velocity{.dx = -0.5f, .dy = -1.0f});

    Entity e3 = world.create();
    world.add_component(e3, Position{.x = 100.0f, .y = 200.0f});

    WorldView<Position, Velocity> view(&world);

    int count = 0;
    for (auto [entity, pos, vel] : view) {
        ASSERT_TRUE(world.has(entity));
        ASSERT_TRUE(world.has_component<Position>(entity));
        ASSERT_TRUE(world.has_component<Velocity>(entity));
        pos->x += vel->dx;
        pos->y += vel->dy;
        ++count;
    }
    EXPECT_EQ(count, 2);

    auto e1_pos = world.get_component<Position>(e1);
    EXPECT_FLOAT_EQ(e1_pos->x, 1.5f);
    EXPECT_FLOAT_EQ(e1_pos->y, 3.0f);

    world.destroy();
}

TEST(WorldIntegrationTest, MultipleEntitiesComponentMutation) {
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
        WorldView<Position, Velocity> moving_view(&world);
        for (auto [entity, pos, vel] : moving_view) {
            pos->x += vel->dx;
            pos->y += vel->dy;
        }
    }

    {
        WorldView<Health> health_view(&world);
        for (auto [entity, hp] : health_view) {
            hp->hp -= 10;
        }
    }

    auto player_pos = world.get_component<Position>(player);
    EXPECT_FLOAT_EQ(player_pos->x, 1.0f);
    auto player_hp = world.get_component<Health>(player);
    EXPECT_EQ(player_hp->hp, 90);

    auto enemy_hp = world.get_component<Health>(enemy);
    EXPECT_EQ(enemy_hp->hp, 40);

    ASSERT_FALSE(world.has_component<Health>(wall));

    world.remove_component<Velocity>(player);
    ASSERT_FALSE(world.has_component<Velocity>(player));
    ASSERT_TRUE(world.has_component<Position>(player));

    world.destroy();
}

auto main(int argc, char **argv) -> int {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
