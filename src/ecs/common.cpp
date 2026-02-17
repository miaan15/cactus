module;

#include <cstddef>
#include <cstdint>
#include <type_traits>

export module Ecs:Common;

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
