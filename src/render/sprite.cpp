module;

#include <boost/container/vector.hpp>
#include <cstdlib>
#include <expected>
#include <filesystem>

export module Render:Sprite;

import :Texture;
import glm;

using namespace glm;
namespace stdf = std::filesystem;
namespace bstc = boost::container;

namespace cactus {

export struct SpriteAtlas {
    struct SpriteDef {
        size_t texture_id;
        mat2x2 srcrect;
    };

    bstc::vector<Texture> textures;
    bstc::vector<SpriteDef> sprite_defs;
};

} // namespace cactus
