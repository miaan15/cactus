module;

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <boost/container/vector.hpp>
#include <cstddef>
#include <cstring>
#include <optional>
#include <utility>
#include <variant>

#include <print>

export module Input;

export import :Define;

import glm;

using namespace glm;
namespace bstc = boost::container;

namespace cactus {

export struct alignas(alignof(std::max_align_t)) InputAction { // FIXME hardcoded
    std::byte data[sizeof(vec2)];
};

export auto is_using(KeyboardKeycode key) -> bool {
    const bool *state = SDL_GetKeyboardState(nullptr);
    return state[static_cast<SDL_Scancode>(key)];
}

template <typename T>
concept InputBindingConcept = requires(T b, InputAction *action) {
    { b.apply_to(action) } -> std::same_as<void>;
};

export struct ButtonInputBinding {
    KeyboardKeycode key;

    auto apply_to(InputAction *action) const -> void {
        bool *v = (bool *)(&action->data);
        if (*v == true) return;
        *v = is_using(key);
    }
};

export struct UDLRInputBinding {
    KeyboardKeycode up_key;
    KeyboardKeycode down_key;
    KeyboardKeycode left_key;
    KeyboardKeycode right_key;

    auto apply_to(InputAction *action) const {
        vec2 *v = (vec2 *)(&action->data);
        if (is_using(up_key)) v->y += 1;
        if (is_using(down_key)) v->y -= 1;
        if (is_using(left_key)) v->x -= 1;
        if (is_using(right_key)) v->x += 1;
        if (v->x < -1) v->x = -1;
        if (v->x > 1) v->x = 1;
        if (v->y < -1) v->y = -1;
        if (v->y > 1) v->y = 1;
    }
};

export using InputBinding = std::variant<ButtonInputBinding, UDLRInputBinding>;

export struct InputSystem {
    using ActionBinding = std::pair<InputAction, bstc::vector<InputBinding>>;

    bstc::vector<ActionBinding> actions;

    [[nodiscard]] auto create_action() -> size_t {
        actions.emplace_back();
        return actions.size() - 1;
    }

    auto add_binding(InputBinding &&binding) {
        assert(!actions.empty());
        actions.back().second.emplace_back(std::move(binding));
    }

    template <typename T>
        requires(sizeof(T) <= sizeof(InputAction::data))
    auto read_as(size_t index) -> std::optional<T> {
        if (index >= actions.size()) return {};
        T result;
        memcpy(&result, &actions[index].first.data, sizeof(T));
        return result;
    }

    auto update() {
        for (auto &p : actions) {
            auto *action = &p.first;

            memset(action, 0, sizeof(decltype(*action)));

            const auto &bindings = p.second;
            for (const auto &b : bindings) {
                auto visitor = [&]<InputBindingConcept T>(const T &binding) {
                    binding.apply_to(action);
                };
                std::visit(visitor, b);
            }
        }
    }
};

} // namespace cactus
