module;

#include <raylib.h>

#undef WHITE

export module raylib;

export namespace rl {
using ::Color;
using ::Rectangle;
using ::Texture2D;
using ::Vector2;

using ::BeginDrawing;
using ::ClearBackground;
using ::CloseWindow;
using ::DrawTexturePro;
using ::EndDrawing;
using ::InitWindow;
using ::LoadTexture;
using ::UnloadTexture;
using ::WindowShouldClose;

constexpr Color WHITE = {255, 255, 255, 255};
} // namespace rl
