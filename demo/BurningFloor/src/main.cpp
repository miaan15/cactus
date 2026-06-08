#include <SDL3/SDL.h>

import cactus;
import std;
import glm;

namespace stdf = std::filesystem;

constexpr int screen_width = 1280;
constexpr int screen_height = 720;

const stdf::path root_dir = stdf::path{__FILE__}.parent_path() / "..";
const stdf::path asset_dir = root_dir / "asset";

struct PlayerSprite {
    SDL_FRect src_rect;
};

struct PlayerParams {
    glm::vec2 pos = {200, 300};
    glm::vec2 move_dir = {0, -1};
    glm::vec2 facing_dir = {0, -1};

    bool is_attacking = false;
    float start_attack_time = 0;

    size_t cur_frame_index = 0;
};

struct PlayerData {
    float move_speed;
    float attack_duration;
};

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("BurningFloor", screen_width, screen_height, 0, &window, &renderer)) {
        std::cerr << "Failed to create window/renderer: " << SDL_GetError() << std::endl;
        return -1;
    }

    stdf::path player_sprite_dir = "image/img_player_00.png";
    std::string tex_path = (asset_dir / player_sprite_dir).string();

    SDL_Texture* player_texture = nullptr;
    SDL_Surface* surface = SDL_LoadPNG(tex_path.c_str());

    if (surface) {
        player_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (player_texture) {
            SDL_SetTextureScaleMode(player_texture, SDL_SCALEMODE_NEAREST);
        }
    }

    if (!player_texture) {
        std::cerr << "Failed to load texture at: " << tex_path << " | Error: " << SDL_GetError() << std::endl;
    }

    auto player_sprites = cactus::DynamicArray<PlayerSprite>::make();
    player_sprites.append(PlayerSprite{.src_rect = {  0, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = { 32, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = { 64, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = { 96, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = {128, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = {160, 0, 32, 32}});

    stdf::path player_data_dir = "data/player.txt";
    auto player_doc = cactus::SpinesDocument::make();
    player_doc.parse(asset_dir / player_data_dir);

    PlayerData player_data{};
    player_data.move_speed = player_doc.pick("move_speed").as<int>()
        .transform_error([](auto p) {std::println("error get move_speed {} {}", (int)p.first, p.second); return p;})
        .value_or(0);
    player_data.attack_duration = player_doc.pick("attack_duration").as<float>()
        .transform_error([](auto p) {std::println("error get attack_duration {} {}", (int)p.first, p.second); return p;})
        .value_or(0);

    PlayerParams player_params{};
    player_params.cur_frame_index = 0;

    bool running = true;
    Uint64 last_time = SDL_GetPerformanceCounter();

    while (running) {
        Uint64 current_time = SDL_GetPerformanceCounter();
        float dt = (float)(current_time - last_time) / SDL_GetPerformanceFrequency();
        last_time = current_time;
        float current_time_sec = (float)SDL_GetTicks() / 1000.0f;

        SDL_Event event;
        bool attack_input = false;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                if (event.key.key == SDLK_Z) attack_input = true;
            }
        }

        const bool* key_state = SDL_GetKeyboardState(nullptr);
        glm::vec2 move_input{0, 0};

        if (key_state[SDL_SCANCODE_UP]) move_input.y += 1;
        if (key_state[SDL_SCANCODE_LEFT]) move_input.x -= 1;
        if (key_state[SDL_SCANCODE_DOWN]) move_input.y -= 1;
        if (key_state[SDL_SCANCODE_RIGHT]) move_input.x += 1;

        if (!player_params.is_attacking && attack_input) {
            player_params.is_attacking = true;
            player_params.start_attack_time = current_time_sec;
        }

        if (!player_params.is_attacking) {
            if (move_input.x != 0.0f || move_input.y != 0.0f) {
                player_params.move_dir = glm::normalize(move_input);
                player_params.facing_dir = player_params.move_dir;

                if (std::abs(player_params.move_dir.x) > .0001f) {
                    player_params.facing_dir.y = 0;
                    player_params.facing_dir.x = player_params.facing_dir.x > 0 ? 1.0f : -1.0f;
                }
            } else {
                player_params.move_dir = {0, 0};
            }
        }
        else {
            if (current_time_sec - player_params.start_attack_time > player_data.attack_duration) {
                player_params.is_attacking = false;
            }
            player_params.move_dir = {0, 0};
        }

        player_params.pos += player_params.move_dir * player_data.move_speed * dt;

        if (std::abs(player_params.facing_dir.x) > 0) player_params.cur_frame_index = 1;
        else if (player_params.facing_dir.y > 0) player_params.cur_frame_index = 2;
        else player_params.cur_frame_index = 0;
        if (player_params.is_attacking) player_params.cur_frame_index += 3;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        SDL_FRect player_src_rect =
            player_sprites.get(player_params.cur_frame_index).value_or(PlayerSprite{}).src_rect;

        SDL_FlipMode flip = SDL_FLIP_NONE;
        if (player_params.facing_dir.x > 0) {
            flip = SDL_FLIP_HORIZONTAL;
        }

        SDL_FRect dest_rect = {
            player_params.pos.x,
            screen_height - player_params.pos.y,
            128.0f,
            128.0f
        };

        SDL_RenderTextureRotated(
            renderer,
            player_texture,
            &player_src_rect,
            &dest_rect,
            0.0,
            nullptr,
            flip
        );

        SDL_RenderPresent(renderer);
    }

    player_doc.destroy();
    player_sprites.destroy();

    if (player_texture) {
        SDL_DestroyTexture(player_texture);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
