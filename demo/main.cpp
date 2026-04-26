#include <cassert>

import cactus.core.strat;
import cactus.core.ecs;
import std;
import raylib;

struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};

struct Health {
    int hp;
};

int main() {
    using namespace cactus;

    std::cout << "Creating world...\n";
    World world = World::make();

    std::cout << "Creating entity...\n";
    Entity e1 = world.create();
    assert(world.has(e1));
    std::cout << "✓ Entity created\n";

    std::cout << "Adding Position component...\n";
    world.add_component(e1, Position{10.0f, 20.0f});
    assert(world.has_component<Position>(e1));
    auto pos = world.get_component<Position>(e1);
    assert(pos.has_value());
    assert((*pos)->x == 10.0f && (*pos)->y == 20.0f);
    std::cout << "✓ Position: (" << (*pos)->x << ", " << (*pos)->y << ")\n";

    std::cout << "Adding Velocity component...\n";
    world.add_component(e1, Velocity{1.0f, -1.0f});
    assert(world.has_component<Velocity>(e1));
    assert(world.has_component<Position>(e1)); // Still has Position
    std::cout << "✓ Velocity added, Position retained\n";

    std::cout << "Modifying Position...\n";
    auto mut_pos = world.get_component<Position>(e1);
    (*mut_pos)->x = 100.0f;
    assert(world.get_component<Position>(e1).value()->x == 100.0f);
    std::cout << "✓ Position modified to: " << (*mut_pos)->x << "\n";

    std::cout << "Adding Health component...\n";
    world.add_component(e1, Health{100});
    assert(world.has_component<Health>(e1));
    std::cout << "✓ Health: " << world.get_component<Health>(e1).value()->hp << "\n";

    std::cout << "Removing Velocity component...\n";
    world.remove_component<Velocity>(e1);
    assert(!world.has_component<Velocity>(e1));
    assert(world.has_component<Position>(e1));
    assert(world.has_component<Health>(e1));
    std::cout << "✓ Velocity removed, others retained\n";

    std::cout << "\n✅ All tests passed!\n";
    world.destroy();
    return 0;
}
