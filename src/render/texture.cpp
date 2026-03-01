module;

#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <boost/container/vector.hpp>
#include <cstddef>
#include <expected>
#include <memory>
#include <optional>
#include <string>

export module Render:Texture;

namespace bstc = boost::container;

namespace cactus {

export using TextureID = size_t;

export struct LoadTextureError {
    std::string message;
    // TODO more
};

export struct TextureAtlas {
    using SDLTextureUniquePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

    SDL_Renderer *renderer;
    bstc::vector<SDLTextureUniquePtr> textures;

    [[nodiscard]] auto get_texture(TextureID id) -> std::optional<SDL_Texture *> {
        if (id >= textures.size()) return {};
        return textures.at(id).get();
    }

    auto set_renderer(SDL_Renderer *r) {
        renderer = r;
    }

    [[nodiscard]] auto load_img(const std::string &dir)
        -> std::expected<TextureID, LoadTextureError> {
        if (!renderer) return std::unexpected{LoadTextureError{"Renderer is missing"}};

        SDL_Texture *texture = IMG_LoadTexture(renderer, dir.c_str());
        if (!texture) {
            return std::unexpected{LoadTextureError{SDL_GetError()}};
        }
        textures.emplace_back(texture, SDL_DestroyTexture);
        return textures.size() - 1;
    }
};

} // namespace cactus
