module;

#include <climits>

export module burningfloor.entity;

import burningfloor.common;

export namespace bf {

struct EntityKey {
    size_t index : (sizeof(size_t) * CHAR_BIT) - CHAR_BIT;
    size_t gen : CHAR_BIT;

    operator size_t() const { return *reinterpret_cast<const size_t *>(this); }
};

enum EntityTag {

};
struct Entity {
    EntityKey key;

    std::bitset<64> tags;
};

} // namespace bf
