module;

#include <boost/container/vector.hpp>
#include <cstddef>

export module Render:Sprite;

import :Texture;
import glm;

using namespace glm;
namespace bstc = boost::container;

namespace cactus {

export using SpriteID = size_t;

export struct SpriteAtlas {
    struct SpriteDef {
        size_t texture_index;
        mat2x2 srcrect;
    };

    bstc::vector<TextureID> texture_ids;
    bstc::vector<SpriteDef> sprite_defs;

    auto add_texture(TextureID id) -> size_t {
        texture_ids.emplace_back(id);
        return texture_ids.size() - 1;
    }
    auto create_sprite(const mat2x2 &srcrect) -> size_t {
        assert(!texture_ids.empty());
        sprite_defs.emplace_back(texture_ids.back(), srcrect);
        return sprite_defs.size() - 1;
    }
};

} // namespace cactus
