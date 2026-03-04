#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <expected>
#include <filesystem>
#include <spdlog/spdlog.h>

import Common;
import Render;

using namespace cactus;
namespace stdf = std::filesystem;

stdf::path cur_dir = cactus::root_dir / "demo/sprite_render";

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
        return 1;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    if (!SDL_CreateWindowAndRenderer("Hello World", 800, 600, 0, &window, &renderer)) {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    auto texture_atlas = std::make_shared<TextureAtlas>();
    texture_atlas->set_renderer(renderer);
    auto sprite_texture_id =
        texture_atlas->load_img(cur_dir / "sprite.png")
            .or_else([](const auto &err) -> std::expected<TextureID, LoadTextureError> {
                spdlog::error("Failed to create texture: {}", err.message);
                return std::unexpected{err};
            })
            .value_or((TextureID)-1);
    if (sprite_texture_id == (TextureID)-1) return -1;

    SpriteAtlas sprite_atlas;
    sprite_atlas.set_texture_atlas(texture_atlas);
    sprite_atlas.add_texture(sprite_texture_id);
    auto sprite_id = sprite_atlas.create_sprite({0, 0, 16, 16});

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        auto *texture = sprite_atlas.get_sprite_texture(sprite_id).value();
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        auto *srcrect = sprite_atlas.get_sprite_srcrect(sprite_id).value();
        auto *_srcrect = (SDL_FRect *)srcrect;
        SDL_FRect _destrect = {40, 40, 120, 120};
        SDL_RenderTexture(renderer, texture, _srcrect, &_destrect);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
