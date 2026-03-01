module;

#include <SDL3/SDL_render.h>
#include <boost/container/vector.hpp>
#include <cstddef>
#include <memory>
#include <optional>

export module Render:Sprite;

import :Texture;
import glm;

using namespace glm;
namespace bstc = boost::container;

namespace cactus {

export using SpriteID = size_t;

export struct SpriteAtlas {
    struct SpriteDef {
        size_t texture_id_pos;
        mat2x2 srcrect;
    };

    std::shared_ptr<TextureAtlas> texture_atlas;
    bstc::vector<TextureID> texture_ids;

    bstc::vector<SpriteDef> sprite_defs;

    [[nodiscard]] auto get_sprite_texture(SpriteID id) -> std::optional<SDL_Texture *> {
        if (id >= texture_ids.size()) return {};
        return texture_atlas->get_texture(texture_ids.at(id));
    }
    [[nodiscard]] auto get_sprite_srcrect(SpriteID id) -> std::optional<mat2x2 *> {
        if (id >= sprite_defs.size()) return {};
        return &sprite_defs.at(id).srcrect;
    }

    auto set_texture_atlas(std::shared_ptr<TextureAtlas> atlas) {
        texture_atlas = std::move(atlas);
    }
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
