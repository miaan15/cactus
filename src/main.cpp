#include <boost/container/vector.hpp>
#include <cstdlib>
#include <ctime>

namespace rl {
#include <raylib.h>
}

import Physics;
import glm;

using namespace glm;
using namespace cactus;
namespace bstc = boost::container;

// Demo parameters
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int SCREEN_FPS = 60;

constexpr int BOX_COUNT = 3600;
constexpr float MIN_BOX_SIZE = 2.0;
constexpr float MAX_BOX_SIZE = 5.0;
constexpr float MIN_BOX_VEL = 10.0;
constexpr float MAX_BOX_VEL = 60.0;
constexpr float BOX_RESTITUTION = 1.0;
constexpr float BOX_FRICTION = 0.0;

auto random_float(float min, float max) -> float {
    float normalized = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return min + normalized * (max - min);
}

auto random_color() -> rl::Color {
    return rl::Color{static_cast<unsigned char>(rand() % 256),
                     static_cast<unsigned char>(rand() % 256),
                     static_cast<unsigned char>(rand() % 256), 255};
}

auto main() -> int {
    rl::InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cactus Physics Demo");
    rl::SetTargetFPS(SCREEN_FPS);
    srand(static_cast<unsigned>(time(nullptr)));

    PhysicsWorld world{};
    world.margin = 1.0f;

    struct BoxData {
        ColliderKey key;
        rl::Color color;
    };
    bstc::vector<BoxData> boxes;

    for (int i = 0; i < BOX_COUNT; i++) {
        float halfsize = random_float(MIN_BOX_SIZE, MAX_BOX_SIZE) * 0.5f;
        vec2 center{random_float(halfsize, SCREEN_WIDTH - halfsize),
                    random_float(halfsize, SCREEN_HEIGHT - halfsize)};
        vec2 vel{random_float(-MAX_BOX_VEL, MAX_BOX_VEL), random_float(-MAX_BOX_VEL, MAX_BOX_VEL)};

        if (length(vel) < MIN_BOX_VEL) {
            vel = normalize(vel) * MIN_BOX_VEL;
        }

        float invmass = 1.0f / (halfsize * halfsize * 4.0f);

        ColliderKey key = world.create(center, vec2(halfsize, halfsize), invmass, BOX_RESTITUTION,
                                       BOX_FRICTION, BOX_FRICTION);
        auto entry = world.get(key);
        entry->vel = vel;

        boxes.push_back({key, random_color()});
    }

    while (!rl::WindowShouldClose()) {
        float dt = rl::GetFrameTime();

        world.update(dt);

        for (auto &box : boxes) {
            auto entry = world.get(box.key);
            vec2 halfexts = entry->coll.halfexts;

            if (entry->coll.center.x - halfexts.x < 0) {
                entry->coll.center.x = halfexts.x;
                entry->vel.x *= -1;
            }
            if (entry->coll.center.x + halfexts.x > SCREEN_WIDTH) {
                entry->coll.center.x = SCREEN_WIDTH - halfexts.x;
                entry->vel.x *= -1;
            }
            if (entry->coll.center.y - halfexts.y < 0) {
                entry->coll.center.y = halfexts.y;
                entry->vel.y *= -1;
            }
            if (entry->coll.center.y + halfexts.y > SCREEN_HEIGHT) {
                entry->coll.center.y = SCREEN_HEIGHT - halfexts.y;
                entry->vel.y *= -1;
            }
        }

        rl::BeginDrawing();
        rl::ClearBackground(rl::RAYWHITE);

        for (const auto &box : boxes) {
            auto entry = world.get(box.key);
            vec2 center = entry->coll.center;
            vec2 halfexts = entry->coll.halfexts;

            rl::DrawRectangle(
                static_cast<int>(center.x - halfexts.x), static_cast<int>(center.y - halfexts.y),
                static_cast<int>(halfexts.x * 2), static_cast<int>(halfexts.y * 2), box.color);

            rl::DrawRectangleLines(
                static_cast<int>(center.x - halfexts.x), static_cast<int>(center.y - halfexts.y),
                static_cast<int>(halfexts.x * 2), static_cast<int>(halfexts.y * 2), rl::BLACK);
        }

        rl::DrawFPS(10, 10);
        rl::EndDrawing();
    }

    rl::CloseWindow();

    return 0;
}
