module;

export module burningfloor.enemy;

import burningfloor.common;
import burningfloor.player;

export namespace bf {

struct EnemySprite {
    size_t texture_index;
    SDL_FRect rect;
};

struct EnemyRenderData {
    SDL_Texture *texture;
    SDL_FRect src_rect;
    SDL_FRect dest_rect;
    SDL_FlipMode flip = SDL_FlipMode::SDL_FLIP_NONE;
};

cact::DynamicArray<SDL_Texture *> enemy_texture_list =
    cact::DynamicArray<SDL_Texture *>::make();

cact::DynamicArray<EnemySprite> enemy_sprite_list =
    cact::DynamicArray<EnemySprite>::make();

struct DummyParams {
    float health;
    glm::vec2 pos;
};

struct DummyData {
    float health;
    float move_speed;
};

enum EnemyEntityType {
    ENEMY_T_DUMMY,
};
struct EnemyEntity {
    EnemyEntityType type;
    EnemyRenderData render_data;
    union {
        DummyParams dummy_params;
    };
};

DummyData dummy_data{};

cact::DynamicArray<EnemyEntity> enemy_entity_list =
    cact::DynamicArray<EnemyEntity>::make();

void handle_enemies_init(SDL_Renderer *renderer) {
    // TEXTURE
    // ========================================================================
    auto handle_fail_surface = [](const stdf::path &path) {
        std::cerr << "Failed to load surface with " << path << " | Error: " << SDL_GetError() << std::endl;
    };
    auto handle_fail_texture = [](const stdf::path &path) {
        std::cerr << "Failed to load texture with " << path << " | Error: " << SDL_GetError() << std::endl;
    };

    // Load dummy tex
    const stdf::path dummy_tex_path = asset_dir / "image/img_enemy_dummy_00.png";
    SDL_Surface *dummy_surface = SDL_LoadPNG(dummy_tex_path.c_str());
    if (dummy_surface) {
        auto texture = SDL_CreateTextureFromSurface(renderer, dummy_surface);
        if (!texture) handle_fail_texture(dummy_tex_path);

        enemy_texture_list.append(texture);

        SDL_DestroySurface(dummy_surface);
    }
    else {
        handle_fail_surface(dummy_tex_path);
    }

    // SPRITE
    // ========================================================================
    enemy_sprite_list.append(EnemySprite{0, {0, 0, 32, 32}});

    // DATA
    // ========================================================================
    auto handle_fail_get_data = [](std::string_view name) {
        return [name](auto p) {
            std::cerr << "Failed to get field [" << name << "] | Error: " << (int)p.first << " : " << p.second << std::endl;
            return p;
        };
    };

    const stdf::path enemy_data_path = asset_dir / "data/enemy.txt";
    auto enemy_doc = cact::SpinesContext::make();
    enemy_doc.parse(enemy_data_path);

    // Dummy data
    dummy_data.health = enemy_doc.pick("dummy").pick("health").as<float>()
        .transform_error(handle_fail_get_data("dummy.health"))
        .value_or(0);
    dummy_data.move_speed = enemy_doc.pick("dummy").pick("move_speed").as<float>()
        .transform_error(handle_fail_get_data("dummy.move_speed"))
        .value_or(0);

    enemy_doc.destroy();

    std::cout << "dummy data: " << "\n";
    std::cout << "+ health:\t" << dummy_data.health << "\n";
    std::cout << "+ move_speed:\t" << dummy_data.move_speed << "\n";

    // TEST
    // ========================================================================
    enemy_entity_list.append(EnemyEntity{
        ENEMY_T_DUMMY,
        {},
        DummyParams{
            .health = dummy_data.health,
            .pos = {100, 100}}
    });
}

void dummy_logic_update(DummyParams *dummy_params, const PlayerParams &player_params) {
    glm::vec2 dir = player_params.pos - dummy_params->pos;
    dir = glm::normalize(dir);
}

void handle_enemies_logic_update() {
}

void handle_enemies_frame_update() {
    for (auto &enemy_entity : enemy_entity_list) {
        EnemyRenderData *render_data = &enemy_entity.render_data;
        switch (enemy_entity.type) {
        case ENEMY_T_DUMMY: {

        } break;
        }
    }
}

void handle_enemies_render(SDL_Renderer *renderer) {
    for (auto enemy_entity : enemy_entity_list) {
        auto render_data = enemy_entity.render_data;
        SDL_RenderTextureRotated(
            renderer,
            render_data.texture,
            &render_data.src_rect,
            &render_data.dest_rect,
            0,
            nullptr,
            render_data.flip
        );
    }
}

void handle_enemies_destroy() {
    for (SDL_Texture *tex : enemy_texture_list) {
        SDL_DestroyTexture(tex);
    }

    enemy_texture_list.destroy();
    enemy_sprite_list.destroy();

    enemy_entity_list.destroy();
}

} // namespace bf
