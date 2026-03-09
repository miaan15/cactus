module;

export module Bundle;

import Ecs;

namespace cactus {

struct Bundle {
    World *world;

    auto set_world(World *w) {
        world = w;
    }

};

};
