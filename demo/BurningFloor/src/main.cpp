import cactus;
import std;
import glm;

int main() {}

// namespace stdf = std::filesystem;
//
// constexpr int screen_width = 1280;
// constexpr int screen_height = 720;
//
// const stdf::path root_dir = stdf::path{__FILE__}.parent_path() / "..";
// const stdf::path asset_dir = root_dir / "asset";
//
// struct PlayerSprite {
//     Rectangle src_rect;
// };
// struct PlayerParams {
//     glm::vec2 pos = {200, 300};
//     glm::vec2 move_dir = {0, -1};
//     glm::vec2 facing_dir = {0, -1};
//
//     bool is_attacking = false;
//     float start_attack_time = 0;
//
//     size_t cur_frame_index = 0;
// };
// struct PlayerData {
//     float move_speed;
//     float attack_duration;
// };
//
// int main() {
//     rl::InitWindow(screen_width, screen_height, "BurningFloor");
//
//     stdf::path player_sprite_dir = "image/img_player_00.png";
//     rl::Texture2D player_texture = rl::LoadTexture((asset_dir / player_sprite_dir).c_str());
//     if (player_texture.id == 0) { std::cerr << "Failed to load texture at: " << (asset_dir / player_sprite_dir) << std::endl; }
//
//     auto player_sprites = cactus::DynamicArray<PlayerSprite>::make();
//     player_sprites.append(PlayerSprite{.src_rect = {  0, 0, 32, 32}});
//     player_sprites.append(PlayerSprite{.src_rect = { 32, 0, 32, 32}});
//     player_sprites.append(PlayerSprite{.src_rect = { 64, 0, 32, 32}});
//     player_sprites.append(PlayerSprite{.src_rect = { 96, 0, 32, 32}});
//     player_sprites.append(PlayerSprite{.src_rect = {128, 0, 32, 32}});
//     player_sprites.append(PlayerSprite{.src_rect = {160, 0, 32, 32}});
//
//     stdf::path player_data_dir = "data/player.txt";
//     auto player_doc = cactus::SpinesDocument::make();
//     player_doc.parse(asset_dir / player_data_dir);
//
//     PlayerData player_data{};
//     player_data.move_speed = player_doc.pick("move_speed").as<int>()
//         .transform_error([](auto p) {std::println("error get move_speed {} {}", (int)p.first, p.second); return p;})
//         .value_or(0);
//     player_data.attack_duration = player_doc.pick("attack_duration").as<float>()
//         .transform_error([](auto p) {std::println("error get attack_duration {} {}", (int)p.first, p.second); return p;})
//         .value_or(0);
//
//     PlayerParams player_params{};
//     player_params.cur_frame_index = 0;
//
//     while (!rl::WindowShouldClose()) {
//         auto dt = rl::GetFrameTime();
//
//         glm::vec2 move_input{0, 0};
//         bool attack_input = false;
//         if (IsKeyDown(KEY_UP)) move_input.y += 1;
//         if (IsKeyDown(KEY_LEFT)) move_input.x -= 1;
//         if (IsKeyDown(KEY_DOWN)) move_input.y -= 1;
//         if (IsKeyDown(KEY_RIGHT)) move_input.x += 1;
//         if (IsKeyPressed(KEY_Z)) attack_input = true;
//
//         if (!player_params.is_attacking && attack_input) {
//             player_params.is_attacking = true;
//             player_params.start_attack_time = GetTime();
//         }
//
//         if (!player_params.is_attacking) {
//             if (move_input.x != 0.0f || move_input.y != 0.0f) {
//                 player_params.move_dir = glm::normalize(move_input);
//                 player_params.facing_dir = player_params.move_dir;
//
//                 if (std::abs(player_params.move_dir.x) > .0001f) {
//                     player_params.facing_dir.y = 0;
//                     player_params.facing_dir.x = player_params.facing_dir.x > 0 ? 1.0f : -1.0f;
//                 }
//             } else {
//                 player_params.move_dir = {0, 0};
//             }
//         }
//         else {
//             if (GetTime() - player_params.start_attack_time > player_data.attack_duration) {
//                 player_params.is_attacking = false;
//             }
//
//             player_params.move_dir = {0, 0};
//         }
//
//         player_params.pos += player_params.move_dir * player_data.move_speed * dt;
//
//         if (std::abs(player_params.facing_dir.x) > 0) player_params.cur_frame_index = 1;
//         else if (player_params.facing_dir.y > 0) player_params.cur_frame_index = 2;
//         else player_params.cur_frame_index = 0;
//         if (player_params.is_attacking) player_params.cur_frame_index += 3;
//
//         rl::BeginDrawing();
//
//         rl::ClearBackground(rl::WHITE);
//
//         Rectangle player_src_rect = 
//             player_sprites.get(player_params.cur_frame_index).value_or(PlayerSprite{}).src_rect;
//         if (player_params.facing_dir.x > 0) player_src_rect.width = -player_src_rect.width;
//
//         rl::DrawTexturePro(
//             player_texture,
//             player_src_rect,
//             Rectangle{player_params.pos.x, screen_height - player_params.pos.y, 128, 128},
//             Vector2{0, 0}, 0, rl::WHITE
//         );
//
//         rl::EndDrawing();
//     }
//
//     player_doc.destroy();
//     player_sprites.destroy();
//
//     rl::UnloadTexture(player_texture);
//
//     rl::CloseWindow();
// }
