#include <SDL3/SDL.h>

import burningfloor.common;
import burningfloor.input;
import burningfloor.player;

using namespace bf;

constexpr int screen_width = 1280;
constexpr int screen_height = 720;

constexpr double logic_update_dt = 0.02; // 50 Hz
constexpr double max_dt = 0.5; // 2 fps

double cur_time_sec = 0.0;

size_t cur_logic_update_count = 0;
double logic_update_accumulator = 0.0;

void logic_update();
void frame_update();

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

    handle_input_init();

    handle_player_init(renderer);

    bool running = true;
    u64 last_time_ns = SDL_GetTicksNS();
    while (running) {
        u64 cur_time_ns = SDL_GetTicksNS();
        double dt = (double)(cur_time_ns - last_time_ns) / 1000000000.0;
        last_time_ns = cur_time_ns;

        if (dt > max_dt) dt = max_dt;

        cur_time_sec += dt;
        logic_update_accumulator += dt;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        handle_input_receive_inputs();

        while (logic_update_accumulator >= logic_update_dt) {
            logic_update();
            handle_input_reset();

            ++cur_logic_update_count;
            logic_update_accumulator -= logic_update_dt;
        }

        double logic_update_alpha = logic_update_accumulator / logic_update_dt; // for interpolate

        frame_update();

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        SDL_RenderTextureRotated(
            renderer,
            player_render_data.texture,
            &player_render_data.src_rect,
            &player_render_data.dest_rect,
            0,
            nullptr,
            player_render_data.flip
        );

        SDL_RenderPresent(renderer);
    }

    handle_input_destroy();

    handle_player_destroy();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void logic_update() {
    handle_player_logic_update(cur_time_sec);
}

void frame_update() {
    handle_player_frame_update();
}
