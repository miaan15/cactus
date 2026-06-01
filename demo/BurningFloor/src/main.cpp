import cactus;
import std;
import raylib;

namespace stdf = std::filesystem;

constexpr int screen_width = 1280;
constexpr int screen_height = 720;

const stdf::path root_dir = stdf::path{__FILE__}.parent_path() / "..";
const stdf::path asset_dir = root_dir / "asset";

int main() {
    rl::InitWindow(screen_width, screen_height, "BurningFloor");

    stdf::path player_sprite_dir = "img/img_player_00.png";
    rl::Texture2D player_texture = rl::LoadTexture((asset_dir / player_sprite_dir).c_str());

    while (!rl::WindowShouldClose()) {
        rl::BeginDrawing();

        rl::ClearBackground(rl::WHITE);

        rl::DrawTexturePro(player_texture, rl::Rectangle{0, 0, 32, 32}, rl::Rectangle{160, 160, 320, 320}, rl::Vector2{16, 16},
                           0, rl::WHITE);

        rl::EndDrawing();
    }

    rl::UnloadTexture(player_texture);

    rl::CloseWindow();
}
