module;

export module cactus.input;

export import :define;

import sdl;
import std;
import glm;
import cactus.common;
import cactus.core.strat;

namespace cact {

const bool *keyboard_state;
auto key_state(Scancode key) noexcept -> bool {
    return keyboard_state[(SDL_Scancode)key];
}

struct alignas(alignof(std::max_align_t)) InputActionData {
    char raw[sizeof(glm::vec2)]; // FIXME maybe not vec2
};

// ==============================================================================
// InputBindings
// ==============================================================================

template <typename T>
concept InputBindingMethodConcept = requires(T b, InputActionData *data) {
    { b.apply_to(data) } -> std::same_as<void>;
};

export struct ButtonInputBindingMethod {
    Scancode key;

    auto apply_to(InputActionData *data) const noexcept {
        bool v{};
        std::memcpy(&v, data, sizeof(bool));

        if (!v) v = key_state(key);

        std::memcpy(data, &v, sizeof(bool));
    }
};

export struct UDLRInputBindingMethod {
    Scancode up_key;
    Scancode down_key;
    Scancode left_key;
    Scancode right_key;

    auto apply_to(InputActionData *data) const noexcept {
        glm::vec2 v{};
        std::memcpy(&v, data, sizeof(glm::vec2));

        if (key_state(up_key)    && v.y <= 0) v.y += 1;
        if (key_state(down_key)  && v.y >= 0) v.y -= 1;
        if (key_state(left_key)  && v.x >= 0) v.x -= 1;
        if (key_state(right_key) && v.x <= 0) v.x += 1;

        if (glm::dot(v, v) >= 0.0001f) v = glm::normalize(v);

        std::memcpy(data, &v, sizeof(glm::vec2));
    }

    [[nodiscard]] static auto make_use_arrows() noexcept -> UDLRInputBindingMethod {
        return UDLRInputBindingMethod{Scancode::UP, Scancode::DOWN, Scancode::LEFT, Scancode::RIGHT};
    }
    [[nodiscard]] static auto make_use_wasd() noexcept -> UDLRInputBindingMethod {
        return UDLRInputBindingMethod{Scancode::W, Scancode::S, Scancode::A, Scancode::D};
    }
};

export using InputBindingMethod
    = std::variant<
        ButtonInputBindingMethod,
        UDLRInputBindingMethod
    >;

// ==============================================================================

export struct InputAction {
    InputActionData data{};
    InputActionData last_data{};
    DynamicArray<InputBindingMethod> methods = DynamicArray<InputBindingMethod>::make();

    [[nodiscard]] static auto make() noexcept -> InputAction { return InputAction{}; }
    auto destroy() noexcept {
        methods.destroy();
    }
    [[nodiscard]] auto clone() const noexcept -> InputAction = delete; // TODO

    auto add_binding_method(InputBindingMethod &&method) noexcept {
        methods.append(std::move(method));
    }

    template <typename T>
        requires(sizeof(T) <= sizeof(InputActionData))
    [[nodiscard]] auto as() noexcept -> T {
        T result{};
        std::memcpy(&result, &data, sizeof(T));
        return result;
    }

    template <typename T>
        requires(sizeof(T) <= sizeof(InputActionData))
    [[nodiscard]] auto last_as() noexcept -> T {
        T result{};
        std::memcpy(&result, &last_data, sizeof(T));
        return result;
    }

    auto receive_inputs() noexcept {
        keyboard_state = SDL_GetKeyboardState(nullptr);

        for (const auto &method : methods) {
            auto visitor = [&]<InputBindingMethodConcept T>(const T &m) {
                m.apply_to(&data);
            };
            std::visit(visitor, method);
        }
    }

    auto reset() noexcept {
        std::memcpy(&last_data, &data, sizeof(InputActionData));
        std::memset(&data, 0, sizeof(InputActionData));
    }
};

} // namespace cact
