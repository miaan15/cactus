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
bool key_state(Scancode key) noexcept {
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

    void apply_to(InputActionData *data) const noexcept {
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

    void apply_to(InputActionData *data) const noexcept {
        glm::vec2 v{};
        std::memcpy(&v, data, sizeof(glm::vec2));

        if (key_state(up_key)    && v.y <= 0) v.y += 1;
        if (key_state(down_key)  && v.y >= 0) v.y -= 1;
        if (key_state(left_key)  && v.x >= 0) v.x -= 1;
        if (key_state(right_key) && v.x <= 0) v.x += 1;

        if (glm::dot(v, v) >= 0.0001f) v = glm::normalize(v);

        std::memcpy(data, &v, sizeof(glm::vec2));
    }

    [[nodiscard]] static UDLRInputBindingMethod make_use_arrows() noexcept {
        return UDLRInputBindingMethod{Scancode::UP, Scancode::DOWN, Scancode::LEFT, Scancode::RIGHT};
    }
    [[nodiscard]] static UDLRInputBindingMethod make_use_wasd() noexcept {
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

    [[nodiscard]] static InputAction make() noexcept { return InputAction{}; }
    void destroy() noexcept {
        methods.destroy();
    }
    [[nodiscard]] InputAction clone() const noexcept = delete; // TODO

    void add_binding_method(InputBindingMethod &&method) noexcept {
        methods.append(std::move(method));
    }

    template <typename T>
        requires(sizeof(T) <= sizeof(InputActionData))
    [[nodiscard]] T as() noexcept {
        T result{};
        std::memcpy(&result, &data, sizeof(T));
        return result;
    }

    template <typename T>
        requires(sizeof(T) <= sizeof(InputActionData))
    [[nodiscard]] T last_as() noexcept {
        T result{};
        std::memcpy(&result, &last_data, sizeof(T));
        return result;
    }

    void receive_inputs() noexcept {
        keyboard_state = SDL_GetKeyboardState(nullptr);

        for (const auto &method : methods) {
            auto visitor = [&]<InputBindingMethodConcept T>(const T &m) {
                m.apply_to(&data);
            };
            std::visit(visitor, method);
        }
    }

    void reset() noexcept {
        std::memcpy(&last_data, &data, sizeof(InputActionData));
        std::memset(&data, 0, sizeof(InputActionData));
    }
};

} // namespace cact
