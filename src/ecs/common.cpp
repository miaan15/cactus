module;

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

export module Ecs:Common;

namespace cactus::ecs {

export using Entity = size_t;
export using Signature = uint64_t;

export template <typename... Ts>
struct is_unique;

export template <typename T>
struct is_unique<T> : std::true_type {};

export template <typename T, typename... Us>
struct is_unique<T, Us...> {
    static constexpr bool value = (!std::is_same_v<T, Us> && ...) && is_unique<Us...>::value;
};

export template <typename... Ts>
constexpr bool is_unique_v = is_unique<Ts...>::value;

export template <typename... Ts>
struct Prefab {
    std::byte *root;

    template <size_t I, typename... Us>
        requires(I < sizeof...(Us))
    struct type_at;
    template <typename U, typename... Us>
    struct type_at<0, U, Us...> {
        using type = U;
    };
    template <size_t I, typename U, typename... Us>
        requires(I < sizeof...(Us) + 1)
    struct type_at<I, U, Us...> {
        using type = typename type_at<I - 1, Us...>::type;
    };

    template <size_t I>
    struct component_at {
        using type = type_at<I, Ts...>::type;
    };
    template <size_t I>
    using component_at_t = component_at<I>::type;

    template <typename T>
    static consteval auto component_index() -> size_t {
        size_t id = 0;
        bool f = false;
        ((std::is_same_v<T, Ts> ? f = true : id += !f), ...);
        return id;
    }

    template <typename T>
    static consteval auto is_contains_component() -> bool {
        return (std::is_same_v<T, Ts> || ...);
    }

    static constexpr size_t align_up(size_t offset, size_t alignment) {
        return (offset + alignment - 1) & ~(alignment - 1);
    }

    static constexpr auto total_size() -> size_t {
        auto cal = [&]<size_t... Is>(std::index_sequence<Is...>) -> size_t {
            size_t offset = 0;
            ((offset = align_up(offset, alignof(component_at_t<Is>)) + sizeof(component_at_t<Is>)),
             ...);
            return offset;
        };
        return cal(std::make_index_sequence<sizeof...(Ts)>{});
    }

    template <size_t I>
    static constexpr auto component_offset() -> size_t {
        auto cal = [&]<size_t... Is>(std::index_sequence<Is...>) -> size_t {
            size_t offset = 0;
            ((offset = align_up(offset, alignof(component_at_t<Is>)) + sizeof(component_at_t<Is>)),
             ...);
            offset = align_up(offset, alignof(component_at_t<I>));
            return offset;
        };
        return cal(std::make_index_sequence<I>{});
    }

    template <typename T>
        requires(is_contains_component<T>())
    inline auto get() -> T * {
        return (T *)(root + component_offset<component_index<T>()>());
    }
    template <size_t I>
        requires(I <= sizeof...(Ts))
    inline auto get() -> component_at_t<I> * {
        return (component_at_t<I> *)(root + component_offset<I>());
    }

    template <typename T>
        requires(is_contains_component<T>())
    inline auto get() const -> const T * {
        return (T *)(root + component_offset<component_index<T>()>());
    }
    template <size_t I>
        requires(I <= sizeof...(Ts))
    inline auto get() const -> const component_at_t<I> * {
        return (component_at_t<I> *)(root + component_offset<I>());
    }
};

} // namespace cactus::ecs
