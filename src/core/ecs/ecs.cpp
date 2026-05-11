module;

#include <cassert>

export module cactus.core.ecs;

// World is the ground that holds entities and datas (Archetypes)
// Entity is just a key to access to Component data
// Component is plain data of a type T
// Signature is a list of std::type_index (type of Component)
// Archetype hold all actual Component data of a single Signature
// .
// Component is represented by std::type_index
// Archetype structured like a table
// Signature is a blueprint of what component Entity/Archetype has

export import :world;

import std;
