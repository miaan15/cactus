# Cactus — Copilot Instructions

Cactus is a 2D game engine written in modern C++23, using C++23 modules for all source organization. It features an archetype-based ECS, an SDL3/SDL3-GPU render system, and a GLM-based physics engine.

## Build & Run

**Prerequisites:** Compiler with C++23 modules support (e.g., Clang 18+, GCC 14+), CMake ≥ 4.1.4.

```bash
# Fetch all vendored dependencies (first time only)
./fetch.sh

# Configure and build
mkdir build && cd build
cmake ..
make

# Build with AddressSanitizer
cmake -DUSE_ASAN=ON ..
make
```

**Run a demo:**
```bash
./build/demo/ecs_demo
./build/demo/sprite_render_demo
```

**Run tests:**
```bash
./build/test/ecs_test
./build/test/slot_map_test

# Run a single GTest filter
./build/test/ecs_test --gtest_filter=WorldTest.EmplaceComponent
```

## Adding Demos and Tests

The CMake build auto-discovers entry points by folder name:

- Create `demo/<name>/main.cpp` → builds as `<name>_demo` → outputs to `build/demo/`
- Create `test/<name>/main.cpp` → builds as `<name>_test` → outputs to `build/test/`

No CMakeLists changes required. All `src/**/*.cpp` modules are automatically included in every target.

## Module Architecture

Every source file under `src/` is a C++23 module compiled into **every** demo and test target. Modules use named partitions:

```cpp
// Aggregator module re-exports all partitions
export module Ecs;
export import :Common;
export import :World;

// Partition implementation
module;               // global module fragment — put #includes here
#include <headers>
export module Ecs:World;
import :Common;       // import sibling partitions
```

**Module map:**

| Module | File | Purpose |
|---|---|---|
| `Ecs` | `src/ecs/ecs.cpp` | Aggregator |
| `Ecs:World` | `src/ecs/world.cpp` | Entity lifecycle, component queries |
| `Ecs:Component` | `src/ecs/component.cpp` | Component type registry (hash-based) |
| `Ecs:Signature` | `src/ecs/signature.cpp` | Sorted component-ID sets, transition cache |
| `Ecs:Archetype` | `src/ecs/archetype.cpp` | SoA table storage per signature |
| `Ecs:Common` | `src/ecs/common.cpp` | `Entity` type, namespace aliases |
| `Render` | `src/render/render.cpp` | Aggregator |
| `Render:Texture` | `src/render/texture.cpp` | SDL3 texture atlas |
| `Render:Sprite` | `src/render/sprite.cpp` | Sprite definitions (texture + source rect) |
| `SlotMap` | `src/data_structure/slot_map.cpp` | Generic slot map container |
| `Common` | `src/common.cpp` | Project-relative `root_dir`, `src_dir` paths |
| `glm` | `vendor/glm/glm/glm.cppm` | Math (imported as a module) |

## ECS Design

Archetype-based (table-driven) ECS with Structure-of-Arrays storage:

- **Entity**: `tuplet::tuple<size_t, size_t>` (index, generation)
- **Component identity**: `std::typeid(T).hash_code()` — no manual ID registration needed
- **Signature**: sorted `vector<ComponentID>` — uniquely identifies a component set
- **ArchetypeTable**: aligned per-component columns + entity ownership list; `emplace_component` and `remove_component` migrate the entity between archetypes

```cpp
cactus::ecs::World world;
auto e = world.create_entity();
world.emplace_component<Position>(e, 0.f, 0.f);
world.emplace_component<Velocity>(e, 1.f, 0.f);
auto* pos = world.get_component<Position>(e).value(); // returns optional<T*>
world.remove_component<Velocity>(e);
```

Signature transitions are cached in `SignatureAtlas` as `{from_id, component_id} → new_id` maps to avoid repeated lookups.

## Key Conventions

**Namespaces:**
- All engine code: `cactus` namespace
- ECS subsystem: `cactus::ecs`
- Boost containers aliased inside `Ecs:Common`: `bstc = boost::container`, `bstu = boost::unordered`
- SDL3 types used directly (no wrapper namespace)

**Types and structs:**
- All types are plain structs with public members — no private sections or getter/setter patterns
- `[[nodiscard]]` on all query/lookup methods
- `std::optional<T*>` for nullable results; `std::expected<T, E>` for fallible operations (e.g., texture loading)
- Avoid `std::vector` in engine code — use `boost::container::vector` instead

**AABB representation:** `glm::mat2x2` where `aabb[0]` = min corner, `aabb[1]` = max corner.

**Asset paths:** Use `cactus::root_dir` / `cactus::src_dir` from the `Common` module for project-relative paths — don't hardcode absolute paths.

**Logging:** Use `spdlog` for all runtime messages (`spdlog::error(...)`, `spdlog::info(...)`).

**Memory:** AABB tree nodes use `malloc`/`free` directly. SDL resources use `std::unique_ptr` with custom deleters via `TextureAtlas`.
