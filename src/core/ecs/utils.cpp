module;

export module cactus.core.ecs:utils;

import cactus.common;

export namespace cact::detail::ecs {

[[nodiscard]] constexpr size_t align_up(size_t offset, size_t align) noexcept {
    return (offset + align - 1) & ~(align - 1);
}

template <typename... Ts>
struct WorldComponentUtilities {
    [[nodiscard]] constexpr
    static size_t count() noexcept { return sizeof...(Ts); }

    template <typename T> [[nodiscard]] constexpr static bool has() noexcept {
        return (std::is_same_v<T, Ts> || ...);
    }
    template <typename T> [[nodiscard]] constexpr static size_t get_index() noexcept {
        size_t i = 0, res = 0;
        (void)((std::is_same_v<T, Ts> ? (res = i, false) : (++i, true)) && ...);
        return res;
    }

    template <size_t I> using component_at_t =
        typename std::tuple_element<I, std::tuple<Ts...>>::type;

    template <size_t I> [[nodiscard]] constexpr static size_t get_size() noexcept {
        static_assert(I < sizeof...(Ts), "Component index out of bounds");
        return sizeof(component_at_t<I>);
    }
    template <size_t I> [[nodiscard]] constexpr static size_t get_align() noexcept {
        static_assert(I < sizeof...(Ts), "Component index out of bounds");
        return alignof(component_at_t<I>);
    }

    [[nodiscard]] constexpr static size_t get_total_size() noexcept {
        size_t res = 0;
        size_t max_align = 1;
        (..., (max_align = std::max(max_align, alignof(Ts)), res = align_up(res, alignof(Ts)) + sizeof(Ts)));
        return align_up(res, max_align);
    }
};

} // namespace cact::detail::ecs
