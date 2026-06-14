module;

export module burningfloor.player;

import burningfloor.common;
import burningfloor.input;

export namespace bf {

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

struct PlayerRenderData {
    SDL_Texture *texture;
    SDL_FRect src_rect;
    SDL_FRect dest_rect;
    SDL_FlipMode flip = SDL_FlipMode::SDL_FLIP_NONE;
};

const stdf::path player_tex_path = asset_dir / "image/img_player_00.png";
const stdf::path player_data_path = asset_dir / "data/player.txt";

SDL_Texture *player_texture = nullptr;

cact::DynamicArray<PlayerSprite> player_sprites = cact::DynamicArray<PlayerSprite>::make();

PlayerData player_data{};
PlayerParams player_params{};

PlayerRenderData player_render_data{};

auto handle_player_init(SDL_Renderer *renderer) {
    // SDL Texture
    SDL_Surface* surface = SDL_LoadPNG(player_tex_path.c_str());

    if (surface) {
        player_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
    }
    else {
        std::cerr << "Failed to load surface with " << player_tex_path << " | Error: " << SDL_GetError() << "\n";
    }

    if (!player_texture) {
        std::cerr << "Failed to load texture with: " << player_tex_path << " | Error: " << SDL_GetError() << "\n";
    }

    SDL_SetTextureScaleMode(player_texture, SDL_ScaleMode::SDL_SCALEMODE_NEAREST);

    // Sprites
    player_sprites.append(PlayerSprite{.src_rect = {  0, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = { 32, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = { 64, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = { 96, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = {128, 0, 32, 32}});
    player_sprites.append(PlayerSprite{.src_rect = {160, 0, 32, 32}});

    // Data
    auto player_doc = cact::SpinesDocument::make();
    player_doc.parse(player_data_path);

    player_data.move_speed = player_doc.pick("move_speed").as<int>()
        .transform_error([](auto p) {std::println("error get move_speed {} {}", (int)p.first, p.second); return p;})
        .value_or(0);
    player_data.attack_duration = player_doc.pick("attack_duration").as<float>()
        .transform_error([](auto p) {std::println("error get attack_duration {} {}", (int)p.first, p.second); return p;})
        .value_or(0);

    player_doc.destroy();

    std::cout << "player data: " << "\n";
    std::cout << "+ move_speed:\t" << player_data.move_speed << "\n";
    std::cout << "+ attack_duration:\t" << player_data.attack_duration << "\n";

    // Params
    player_params.cur_frame_index = 0;
}

auto handle_player_logic_update(double cur_time_sec) {
    bool attack_input_down = attack_input_action.as<bool>() && !attack_input_action.last_as<bool>();
    glm::vec2 move_input = move_input_action.as<glm::vec2>();

    if (!player_params.is_attacking && attack_input_down) {
        player_params.is_attacking = true;
        player_params.start_attack_time = cur_time_sec;
    }

    if (!player_params.is_attacking) {
        if (glm::length(move_input) > 0.001f) {
            player_params.move_dir = glm::normalize(move_input);
            player_params.facing_dir = player_params.move_dir;

            if (std::abs(player_params.move_dir.x) > .001f) {
                player_params.facing_dir.y = 0;
                player_params.facing_dir.x = player_params.facing_dir.x > 0 ? 1.0f : -1.0f;
            }
        } else {
            player_params.move_dir = {0, 0};
        }
    }
    else {
        if (cur_time_sec - player_params.start_attack_time > player_data.attack_duration) {
            player_params.is_attacking = false;
        }
        player_params.move_dir = {0, 0};
    }

    player_params.pos += player_params.move_dir * player_data.move_speed * 0.02f;

    if (std::abs(player_params.facing_dir.x) > 0) player_params.cur_frame_index = 1;
    else if (player_params.facing_dir.y > 0) player_params.cur_frame_index = 2;
    else player_params.cur_frame_index = 0;
    if (player_params.is_attacking) player_params.cur_frame_index += 3;
}

auto handle_player_frame_update() {
    player_render_data.texture = player_texture;

    player_render_data.src_rect =
        player_sprites.get(player_params.cur_frame_index).value_or(PlayerSprite{}).src_rect;

    player_render_data.dest_rect = { player_params.pos.x, 720.0f - player_params.pos.y, 128.0f, 128.0f };

    player_render_data.flip = player_params.facing_dir.x > 0 ? SDL_FlipMode::SDL_FLIP_HORIZONTAL : SDL_FlipMode::SDL_FLIP_NONE;
}

auto handle_player_destroy() {
    if (player_texture) SDL_DestroyTexture(player_texture);
    player_sprites.destroy();
}

} // namespace bf
