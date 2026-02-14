module;

#include <boost/container/vector.hpp>
#include <cstddef>
#include <cstdint>
#include <flat_map>
#include <type_traits>

export module ECS;

import SlotMap;
import Pool;

namespace cactus::ecs {

export using Entity = uint64_t;

template <typename... Ts>
struct is_unique;

template <typename T>
struct is_unique<T> : std::true_type {};

template <typename T, typename... Us>
struct is_unique<T, Us...> {
    static constexpr bool value = (!std::is_same_v<T, Us> && ...) && is_unique<Us...>::value;
};

template <typename... Ts>
constexpr bool is_unique_v = is_unique<Ts...>::value;

export template <typename... Ts>
    requires is_unique_v<Ts...> && (sizeof...(Ts) <= 64)
struct SmallWorld {
    using Signature = uint64_t;
    struct EntitySpec {
        Signature signature;
        size_t row;
    };
    struct ArchetypeTable {
        std::byte *data_ptr;
        size_t size;
        size_t capacity;
        size_t prefab_size;
    };

    Pool<EntitySpec> entity_specs;
    std::flat_map<Signature, ArchetypeTable> archetypes;

    template <typename T>
    static consteval auto component_id() -> size_t {
        size_t id = 0;
        ((std::is_same_v<T, Ts> ? false : ++id), ...);
        return id;
    }

    template <typename T>
    static consteval auto is_components_contain() -> bool {
        bool res = false;
        res = (std::is_same_v<T, Ts> | ...);
        return res;
    }

    [[nodiscard]] auto create_entity() -> Entity {
        auto entity = entity_specs.emplace();
        return entity;
    }

    template <size_t I, typename... Us>
    struct type_at;
    template <size_t I, typename U, typename... Us>
    struct type_at<I, U, Us...> {
        using type = std::conditional_t<I == 0, U, typename type_at<I - 1, Us...>::type>;
    };

    template <size_t Id>
    struct component_of {
        using type = type_at<Id, Ts...>::type;
    };
    template <size_t Id>
    using component_of_t = component_of<Id>::type;

    template <size_t Id>
    static consteval auto component_size() -> size_t {
        return sizeof(component_of_t<Id>);
    }

    template <size_t Id, Signature S>
    static consteval auto component_offset() -> size_t {
        size_t offset = 0;
        for (auto i = 0; i < Id; ++i) offset += ((S >> i) & 1) != 0 ? component_size<i>() : 0;
        return offset;
    }

    template <typename T>
        requires(is_components_contain<T>())
    auto get(Entity entity) -> std::optional<T *> {
        auto component = component_id<T>();
        auto signature = entity_specs[entity].signature;

        if (((signature >> component) & 1) == 0) return {};

        auto &archetype = archetypes[signature];
        auto column = component_offset<component, signature>();
        auto row = entity_specs[entity].row;

        return (T *)((archetype.ptr + row * archetype.prefab_size) + column);
    }

    template <typename T, typename... Args>
        requires(is_components_contain<T>())
    auto emplace(Entity entity, Args... args) {
        auto component = component_id<T>();
        auto signature = entity_specs[entity].signature;
        auto column = component_offset<component, signature>();

        if (((signature >> component_id<T>()) & 1) != 0) [[unlikely]] {
            auto &archetype = archetypes[signature];
            *(T *)((archetype.ptr + entity_specs[entity].row * archetype.prefab_size)
                   + column) = {std::forward<Args>(args)...};
        } else [[likely]] {
            auto new_signature = signature | (1 << component);
            auto &old_archetype = archetypes[signature];
            auto &new_archetype = archetypes[new_signature];

            auto old_row_ptr =
                old_archetype.ptr + entity_specs[entity].row * old_archetype.prefab_size;
            auto new_row_ptr = new_archetype.ptr + new_archetype.size * new_archetype.prefab_size;

            ++new_archetype.size;
            if (new_archetype.size > new_archetype.capacity) {
                new_archetype.capacity = new_archetype.capacity + (new_archetype.capacity >> 1);
                new_archetype.ptr =
                    realloc(new_archetype.ptr, new_archetype.capacity * new_archetype.prefab_size);
            }

            memcpy(old_archetype, new_archetype, column);
            *(T *)((new_archetype.ptr + entity_specs[entity].row * new_archetype.prefab_size)
                   + column) = {std::forward<Args>(args)...};
            memcpy(old_archetype + column, new_archetype + column + sizeof(T),
                   new_archetype.prefab_size - (column + sizeof(T)));
        }
    }

    template <typename T>
        requires(is_components_contain<T>())
    auto erase(Entity entity) -> void {
        auto signature = entity_specs[entity].signature;
        auto component = component_id<T>();

        if (((signature >> component) & 1) == 0) return;

        auto column = component_offset<component, signature>();
        auto new_signature = signature & ~(1 << component);
        auto &old_archetype = archetypes[signature];
        auto &new_archetype = archetypes[new_signature];

        auto old_row_ptr = old_archetype.ptr + entity_specs[entity].row * old_archetype.prefab_size;
        auto new_row_ptr = new_archetype.ptr + new_archetype.size * new_archetype.prefab_size;

        ++new_archetype.size;
        if (new_archetype.size > new_archetype.capacity) {
            new_archetype.capacity = new_archetype.capacity + (new_archetype.capacity >> 1);
            new_archetype.ptr =
                realloc(new_archetype.ptr, new_archetype.capacity * new_archetype.prefab_size);
        }

        memcpy(old_archetype, new_archetype, column);
        memcpy(old_archetype + column + sizeof(T), new_archetype + column,
               old_archetype.prefab_size - (column + sizeof(T)));

        auto last_old_row_ptr = old_archetype.ptr + old_archetype.size * old_archetype.prefab_size;
        memcpy(last_old_row_ptr, old_row_ptr, old_archetype.prefab_size);
        old_archetype.size--;
    }
};

} // namespace cactus::ecs
