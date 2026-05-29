module;

export module cactus.core.ecs:utils;

import :defines;
import cactus.common;

namespace cactus::detail::ecs {

export [[nodiscard]] constexpr auto align_up(size_t offset, size_t align) -> size_t {
    return (offset + align - 1) & ~(align - 1);
}

export template <typename... Ts>
    requires(sizeof...(Ts) <= MAX_WORLD_COMPONENTS_COUNT && sizeof...(Ts) > 0)
struct WorldComponentUtilities {
    [[nodiscard]] static constexpr auto count() -> size_t { return sizeof...(Ts); }

    template <typename T> [[nodiscard]] static constexpr auto has() -> bool { return (std::is_same_v<T, Ts> || ...); }
    template <typename T> [[nodiscard]] static constexpr auto get_index() -> size_t {
        size_t i = 0, res = 0;
        (void)((std::is_same_v<T, Ts> ? (res = i, false) : (++i, true)) && ...);
        return res;
    }

    template <size_t I> using component_at_t = typename std::tuple_element<I, std::tuple<Ts...>>::type;

    template <size_t I> [[nodiscard]] static constexpr auto get_size() -> size_t {
        static_assert(I < sizeof...(Ts), "Component index out of bounds");
        return sizeof(component_at_t<I>);
    }
    template <size_t I> [[nodiscard]] static constexpr auto get_align() -> size_t {
        static_assert(I < sizeof...(Ts), "Component index out of bounds");
        return alignof(component_at_t<I>);
    }

    [[nodiscard]] static constexpr auto get_total_size() -> size_t {
        size_t res = 0;
        size_t max_align = 1;
        (..., (max_align = std::max(max_align, alignof(Ts)), res = align_up(res, alignof(Ts)) + sizeof(Ts)));
        return align_up(res, max_align);
    }
};

} // namespace cactus::detail::ecs
