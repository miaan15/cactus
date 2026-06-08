module;

export module cactus.input;

export import :define;

import sdl;
import std;
import glm;
import cactus.common;
import cactus.core.strat;

namespace cactus {

struct alignas(alignof(std::max_align_t)) InputActionData {
    char raw[sizeof(glm::vec2)]; // FIXME maybe not vec2
};

export auto is_using(Scancode key) noexcept -> bool {
    const bool *state = SDL_GetKeyboardState(nullptr);
    return state[(SDL_Scancode)key];
}

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

        if (!v) v = is_using(key);

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

        if (is_using(up_key)) v.y += 1;
        if (is_using(down_key)) v.y -= 1;
        if (is_using(left_key)) v.x -= 1;
        if (is_using(right_key)) v.x += 1;
        if (v.x < -1) v.x = -1;
        if (v.x > 1) v.x = 1;
        if (v.y < -1) v.y = -1;
        if (v.y > 1) v.y = 1;

        v = glm::normalize(v);

        std::memcpy(data, &v, sizeof(glm::vec2));
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

    auto update() noexcept {
        std::memcpy(&last_data, &data, sizeof(InputActionData));
        std::memset(&data, 0, sizeof(InputActionData));

        for (const auto &method : methods) {
            auto visitor = [&]<InputBindingMethodConcept T>(const T &m) {
                m.apply_to(&data);
            };
            std::visit(visitor, method);
        }
    }
};

} // namespace cactus
