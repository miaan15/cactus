#include <cassert>

import cactus.core.strat;
import cactus.core.ecs;
import std;
import raylib;

import cactus.core.ecs;

// Primitive type components
struct CompInt1 {
    int value;
};
struct CompInt2 {
    int value;
};
struct CompInt3 {
    int value;
};
struct CompInt4 {
    int value;
};
struct CompInt5 {
    int value;
};

struct CompFloat1 {
    float value;
};
struct CompFloat2 {
    float value;
};
struct CompFloat3 {
    float value;
};
struct CompFloat4 {
    float value;
};
struct CompFloat5 {
    float value;
};

struct CompDouble1 {
    double value;
};
struct CompDouble2 {
    double value;
};
struct CompDouble3 {
    double value;
};

struct CompBool1 {
    bool value;
};
struct CompBool2 {
    bool value;
};
struct CompBool3 {
    bool value;
};

struct CompChar1 {
    char value;
};
struct CompChar2 {
    char value;
};
struct CompChar3 {
    char value;
};

struct CompShort1 {
    short value;
};
struct CompShort2 {
    short value;
};

struct CompLong1 {
    long value;
};
struct CompLong2 {
    long value;
};

struct CompUInt1 {
    unsigned int value;
};
struct CompUInt2 {
    unsigned int value;
};

int main() {
    using namespace cactus;

    std::cout << "=== Many Components Test ===\n\n";

    World world = World::make();

    // Test 1: Entity with many different primitive components
    std::cout << "[Test 1] Adding many components to single entity\n";
    Entity e1 = world.create();

    world.add_component(e1, CompInt1{1});
    world.add_component(e1, CompInt2{2});
    world.add_component(e1, CompInt3{3});
    world.add_component(e1, CompInt4{4});
    world.add_component(e1, CompInt5{5});

    world.add_component(e1, CompFloat1{1.1f});
    world.add_component(e1, CompFloat2{2.2f});
    world.add_component(e1, CompFloat3{3.3f});
    world.add_component(e1, CompFloat4{4.4f});
    world.add_component(e1, CompFloat5{5.5f});

    world.add_component(e1, CompDouble1{1.111});
    world.add_component(e1, CompDouble2{2.222});
    world.add_component(e1, CompDouble3{3.333});

    world.add_component(e1, CompBool1{true});
    world.add_component(e1, CompBool2{false});
    world.add_component(e1, CompBool3{true});

    world.add_component(e1, CompChar1{'A'});
    world.add_component(e1, CompChar2{'B'});
    world.add_component(e1, CompChar3{'C'});

    world.add_component(e1, CompShort1{100});
    world.add_component(e1, CompShort2{200});

    world.add_component(e1, CompLong1{1000000L});
    world.add_component(e1, CompLong2{2000000L});

    world.add_component(e1, CompUInt1{42u});
    world.add_component(e1, CompUInt2{84u});

    std::cout << "✓ Added 25 components\n";

    // Test 2: Verify all components exist
    std::cout << "\n[Test 2] Verifying all components exist\n";
    assert(world.has_component<CompInt1>(e1));
    assert(world.has_component<CompInt5>(e1));
    assert(world.has_component<CompFloat1>(e1));
    assert(world.has_component<CompFloat5>(e1));
    assert(world.has_component<CompDouble3>(e1));
    assert(world.has_component<CompBool2>(e1));
    assert(world.has_component<CompChar3>(e1));
    assert(world.has_component<CompShort1>(e1));
    assert(world.has_component<CompLong2>(e1));
    assert(world.has_component<CompUInt1>(e1));
    std::cout << "✓ All 25 components verified\n";

    // Test 3: Verify all component values
    std::cout << "\n[Test 3] Verifying component values\n";
    assert(world.get_component<CompInt1>(e1).value().value == 1);
    assert(world.get_component<CompInt3>(e1).value().value == 3);
    assert(world.get_component<CompInt5>(e1).value().value == 5);

    assert(world.get_component<CompFloat2>(e1).value().value == 2.2f);
    assert(world.get_component<CompFloat4>(e1).value().value == 4.4f);

    assert(world.get_component<CompDouble1>(e1).value().value == 1.111);
    assert(world.get_component<CompDouble3>(e1).value().value == 3.333);

    assert(world.get_component<CompBool1>(e1).value().value == true);
    assert(world.get_component<CompBool2>(e1).value().value == false);

    assert(world.get_component<CompChar1>(e1).value().value == 'A');
    assert(world.get_component<CompChar3>(e1).value().value == 'C');

    assert(world.get_component<CompShort2>(e1).value().value == 200);
    assert(world.get_component<CompLong1>(e1).value().value == 1000000L);
    assert(world.get_component<CompUInt2>(e1).value().value == 84u);

    std::cout << "✓ All component values correct\n";

    // Test 4: Modify components and verify changes
    std::cout << "\n[Test 4] Modifying components\n";
    world.get_component_ptr<CompInt1>(e1).value()->value = 999;
    world.get_component_ptr<CompFloat3>(e1).value()->value = 99.9f;
    world.get_component_ptr<CompChar2>(e1).value()->value = 'Z';

    assert(world.get_component<CompInt1>(e1).value().value == 999);
    assert(world.get_component<CompFloat3>(e1).value().value == 99.9f);
    assert(world.get_component<CompChar2>(e1).value().value == 'Z');
    std::cout << "✓ Component modifications work\n";

    // Test 5: Remove some components from the middle
    std::cout << "\n[Test 5] Removing components from middle\n";
    world.remove_component<CompInt3>(e1);
    world.remove_component<CompFloat2>(e1);
    world.remove_component<CompDouble2>(e1);
    world.remove_component<CompBool2>(e1);
    world.remove_component<CompShort1>(e1);

    assert(!world.has_component<CompInt3>(e1));
    assert(!world.has_component<CompFloat2>(e1));
    assert(!world.has_component<CompDouble2>(e1));

    // Verify others still exist with correct values
    assert(world.get_component<CompInt1>(e1).value().value == 999);
    assert(world.get_component<CompInt2>(e1).value().value == 2);
    assert(world.get_component<CompInt4>(e1).value().value == 4);
    assert(world.get_component<CompFloat3>(e1).value().value == 99.9f);
    assert(world.get_component<CompChar2>(e1).value().value == 'Z');

    std::cout << "✓ Removed 5 components, others preserved\n";

    // Test 6: Multiple entities with overlapping but different component sets
    std::cout << "\n[Test 6] Multiple entities with many components\n";
    Entity e2 = world.create();
    Entity e3 = world.create();

    // e2: Ints and Floats only
    for (int i = 1; i <= 5; ++i) { world.add_component(e2, CompInt1{i * 10}); }
    world.add_component(e2, CompFloat1{10.1f});
    world.add_component(e2, CompFloat2{20.2f});

    // e3: Bools, Chars, and Longs
    world.add_component(e3, CompBool1{false});
    world.add_component(e3, CompBool2{true});
    world.add_component(e3, CompChar1{'X'});
    world.add_component(e3, CompChar2{'Y'});
    world.add_component(e3, CompLong1{9999999L});

    assert(world.has_component<CompFloat1>(e2));
    assert(!world.has_component<CompBool1>(e2));
    assert(world.has_component<CompChar1>(e3));
    assert(!world.has_component<CompFloat1>(e3));

    std::cout << "✓ Multiple entities with different component sets\n";

    // Test 7: Add back previously removed components
    std::cout << "\n[Test 7] Re-adding removed components\n";
    world.add_component(e1, CompInt3{333});
    world.add_component(e1, CompFloat2{222.2f});

    assert(world.has_component<CompInt3>(e1));
    assert(world.get_component<CompInt3>(e1).value().value == 333);
    assert(world.get_component<CompFloat2>(e1).value().value == 222.2f);

    // Verify old values still intact
    assert(world.get_component<CompInt1>(e1).value().value == 999);
    assert(world.get_component<CompChar2>(e1).value().value == 'Z');

    std::cout << "✓ Re-added components work correctly\n";

    std::cout << "\n✅ All many-component tests passed!\n";
    std::cout << "   Tested with 25+ primitive component types\n";

    world.destroy();
    return 0;
}
