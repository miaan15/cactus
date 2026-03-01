module;

#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <boost/container/vector.hpp>
#include <cstddef>
#include <expected>
#include <memory>
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

    std::shared_ptr<SDL_Renderer> renderer;
    bstc::vector<SDLTextureUniquePtr> textures;

    auto set_renderer(std::shared_ptr<SDL_Renderer> r) {
        renderer = std::move(r);
    }

    auto load_img(const std::string &dir) -> std::expected<TextureID, LoadTextureError> {
        SDL_Surface *surface = IMG_Load(dir.c_str());
        if (!surface) {
            return std::unexpected{LoadTextureError{SDL_GetError()}};
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer.get(), surface);
        if (!texture) {
            return std::unexpected{LoadTextureError{SDL_GetError()}};
        }

        SDL_DestroySurface(surface);

        textures.emplace_back(texture, SDL_DestroyTexture);
        return textures.size() - 1;
    }
};

} // namespace cactus
