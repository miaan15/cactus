module;

#include <raylib.h>

#undef WHITE
#undef DARKGRAY

export module raylib;

export namespace rl {
using ::Color;
using ::Rectangle;
using ::Texture2D;
using ::Vector2;

using ::BeginDrawing;
using ::ClearBackground;
using ::CloseWindow;
using ::DrawText;
using ::DrawTexturePro;
using ::EndDrawing;
using ::GetFrameTime;
using ::InitWindow;
using ::IsKeyDown;
using ::LoadTexture;
using ::UnloadTexture;
using ::WindowShouldClose;

constexpr Color WHITE = {255, 255, 255, 255};
constexpr Color DARKGRAY = {80, 80, 80, 255};
} // namespace rl
