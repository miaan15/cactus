#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

import Input;
import glm;

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Input Demo", 640, 480, 0, &window, &renderer)) {
        std::cerr << "SDL_CreateWindowAndRenderer failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    cactus::InputSystem input;

    size_t move_action = input.create_action();
    input.add_binding(cactus::UDLRInputBinding{
        .up_key = cactus::KeyboardKeycode::W,
        .down_key = cactus::KeyboardKeycode::S,
        .left_key = cactus::KeyboardKeycode::A,
        .right_key = cactus::KeyboardKeycode::D,
    });
    input.add_binding(cactus::UDLRInputBinding{
        .up_key = cactus::KeyboardKeycode::UP,
        .down_key = cactus::KeyboardKeycode::DOWN,
        .left_key = cactus::KeyboardKeycode::LEFT,
        .right_key = cactus::KeyboardKeycode::RIGHT,
    });

    size_t jump_action = input.create_action();
    input.add_binding(cactus::ButtonInputBinding{.key = cactus::KeyboardKeycode::SPACE});
    input.add_binding(cactus::ButtonInputBinding{.key = cactus::KeyboardKeycode::Z});

    std::cout << "Input demo running.\n"
              << "  WASD  or ARROWS = movement\n"
              << "  SPACE or Z      = jump\n"
              << "  ESC             = quit\n";

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
                running = false;
        }

        input.update();

        auto move = input.read_as<glm::vec2>(move_action).value_or(glm::vec2{0, 0});
        auto jump = input.read_as<bool>(jump_action).value_or(false);

        if (move.x != 0 || move.y != 0 || jump) {
            std::cout << "move=(" << move.x << ", " << move.y << ")  jump=" << jump << "\n";
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
