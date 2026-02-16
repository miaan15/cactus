module;

#include <boost/container/vector.hpp>
#include <cstddef>
#include <cstdint>
#include <flat_map>
#include <type_traits>

export module ECS;

import SlotMap;
import FreelistVector;

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

export template <typename... Ts>
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
        std::byte *ptr;
        size_t size;
        size_t capacity;
        size_t prefab_size;
    };

    FreelistVector<EntitySpec> entity_specs;
    std::flat_map<Signature, ArchetypeTable> archetypes;

    template <typename T>
    static consteval auto component_id() -> size_t {
        size_t id = 0;
        bool f = false;
        ((std::is_same_v<T, Ts> ? f = true : id += !f), ...);
        return id;
    }

    template <typename T>
    static consteval auto is_components_contain() -> bool {
        bool res = false;
        res = (std::is_same_v<T, Ts> | ...);
        return res;
    }

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
        using type = std::conditional_t<I == 0, U, typename type_at<I - 1, Us...>::type>;
    };

    template <size_t Id>
    struct component_at_id {
        using type = type_at<Id, Ts...>::type;
    };
    template <size_t Id>
    using component_at_id_t = component_at_id<Id>::type;

    template <size_t Id>
    static consteval auto component_size() -> size_t {
        return sizeof(component_at_id_t<Id>);
    }

    constexpr auto archetype_prefab_size(Signature signature) -> size_t {
        auto cal = [&]<size_t... Is>(std::index_sequence<Is...>) -> size_t {
            return (size_t(0) + ... + ((signature >> Is) & 1 ? component_size<Is>() : 0));
        };
        return cal(std::make_index_sequence<sizeof...(Ts)>{});
    }

    template <size_t Id>
    constexpr auto component_offset(Signature signature) -> size_t {
        auto cal = [&]<size_t... Is>(std::index_sequence<Is...>) -> size_t {
            return (size_t(0) + ... + ((signature >> Is) & 1 ? component_size<Is>() : 0));
        };
        return cal(std::make_index_sequence<Id>{});
    }

    [[nodiscard]] auto create_entity() -> Entity {
        auto entity = entity_specs.emplace();
        return entity;
    }

    template <typename T>
        requires(is_components_contain<T>())
    auto get(Entity entity) -> std::optional<T *> {
        constexpr auto component = component_id<T>();
        const auto signature = entity_specs[entity].signature;

        if (((signature >> component) & 1) == 0) return {};

        assert(archetypes.contains(signature));
        auto archetype = &archetypes.at(signature);
        const auto column = component_offset<component>(signature);
        const auto row = entity_specs[entity].row;

        return (T *)((archetype->ptr + row * archetype->prefab_size) + column);
    }

    template <typename T, typename... Args>
        requires(is_components_contain<T>())
    auto emplace(Entity entity, Args... args) {
        constexpr auto component = component_id<T>();
        const auto signature = entity_specs[entity].signature;

        if (((signature >> component_id<T>()) & 1) != 0) [[unlikely]] {
            assert(archetypes.contains(signature));
            auto archetype = &archetypes.at(signature);
            const auto column = component_offset<component>(signature);

            *(T *)((archetype->ptr + entity_specs[entity].row * archetype->prefab_size)
                   + column) = {std::forward<Args>(args)...};

            entity_specs[entity].signature = signature;
            entity_specs[entity].row = archetype->size - 1;
        } else [[likely]] {
            const auto new_signature = signature | (1 << component);
            const auto column = component_offset<component>(new_signature);

            if (signature == 0) {
                create_archetype_if_needed(new_signature);
                auto new_archetype = &archetypes.at(new_signature);

                ++new_archetype->size;
                if (new_archetype->size > new_archetype->capacity) {
                    new_archetype->capacity =
                        new_archetype->capacity + (new_archetype->capacity >> 1);
                    if (new_archetype->capacity < 2) new_archetype->capacity = 2;
                    new_archetype->ptr = (std::byte *)realloc(
                        new_archetype->ptr, new_archetype->capacity * new_archetype->prefab_size);
                }

                auto new_row_ptr =
                    new_archetype->ptr + (new_archetype->size - 1) * new_archetype->prefab_size;

                std::construct_at((T *)new_row_ptr, std::forward<Args>(args)...);

                entity_specs[entity].signature = new_signature;
                entity_specs[entity].row = new_archetype->size - 1;

                return;
            }

            assert(archetypes.contains(signature));
            create_archetype_if_needed(new_signature);
            auto old_archetype = &archetypes.at(signature);
            auto new_archetype = &archetypes.at(new_signature);

            ++new_archetype->size;
            if (new_archetype->size > new_archetype->capacity) {
                new_archetype->capacity = new_archetype->capacity + (new_archetype->capacity >> 1);
                if (new_archetype->capacity < 2) new_archetype->capacity = 2;
                new_archetype->ptr = (std::byte *)realloc(
                    new_archetype->ptr, new_archetype->capacity * new_archetype->prefab_size);
            }

            const auto old_row = entity_specs[entity].row;

            auto old_row_ptr = old_archetype->ptr + old_row * old_archetype->prefab_size;
            auto new_row_ptr =
                new_archetype->ptr + (new_archetype->size - 1) * new_archetype->prefab_size;

            memcpy(new_row_ptr, old_row_ptr, column);
            std::construct_at((T *)new_row_ptr + column, std::forward<Args>(args)...);
            memcpy(new_row_ptr + column + sizeof(T), old_row_ptr + column,
                   old_archetype->prefab_size - column);

            const auto last_row = old_archetype->size - 1;
            if (old_row != last_row) {
                auto last_old_row_ptr = old_archetype->ptr + last_row * old_archetype->prefab_size;
                memcpy(old_row_ptr, last_old_row_ptr, old_archetype->prefab_size);
            }
            --old_archetype->size;

            entity_specs[entity].signature = new_signature;
            entity_specs[entity].row = new_archetype->size - 1;
        }
    }

    template <typename T>
        requires(is_components_contain<T>())
    auto erase(Entity entity) -> void {
        constexpr auto component = component_id<T>();
        const auto signature = entity_specs[entity].signature;

        if (((signature >> component) & 1) == 0) return;

        const auto new_signature = signature & ~(1 << component);

        if (new_signature == 0) {
            assert(archetypes.contains(signature));
            auto old_archetype = &archetypes.at(signature);

            const auto old_row = entity_specs[entity].row;
            const auto last_row = old_archetype->size - 1;
            if (old_row != last_row) {
                auto old_row_ptr = old_archetype->ptr + old_row * old_archetype->prefab_size;
                auto last_old_row_ptr = old_archetype->ptr + last_row * old_archetype->prefab_size;

                memcpy(old_row_ptr, last_old_row_ptr, old_archetype->prefab_size);
            }
            --old_archetype->size;

            return;
        }

        const auto column = component_offset<component>(signature);

        assert(archetypes.contains(signature));
        create_archetype_if_needed(new_signature);
        auto old_archetype = &archetypes.at(signature);
        auto new_archetype = &archetypes.at(new_signature);

        ++new_archetype->size;
        if (new_archetype->size > new_archetype->capacity) {
            new_archetype->capacity = new_archetype->capacity + (new_archetype->capacity >> 1);
            if (new_archetype->capacity < 2) new_archetype->capacity = 2;
            new_archetype->ptr = (std::byte *)realloc(
                new_archetype->ptr, new_archetype->capacity * new_archetype->prefab_size);
        }

        const auto old_row = entity_specs[entity].row;

        auto old_row_ptr = old_archetype->ptr + old_row * old_archetype->prefab_size;
        auto new_row_ptr =
            new_archetype->ptr + (new_archetype->size - 1) * new_archetype->prefab_size;

        memcpy(new_row_ptr, old_row_ptr, column);
        memcpy(new_row_ptr + column, old_row_ptr + column + sizeof(T),
               new_archetype->prefab_size - column);

        const auto last_row = old_archetype->size - 1;
        if (old_row != last_row) {
            auto last_old_row_ptr = old_archetype->ptr + last_row * old_archetype->prefab_size;
            memcpy(old_row_ptr, last_old_row_ptr, old_archetype->prefab_size);
        }
        --old_archetype->size;

        entity_specs[entity].signature = new_signature;
        entity_specs[entity].row = new_archetype->size - 1;
    }

    auto create_archetype_if_needed(Signature signature) {
        if (!archetypes.contains(signature)) {
            const auto prefab_size = archetype_prefab_size(signature);
            auto p = archetypes
                         .emplace(signature, ArchetypeTable{(std::byte *)malloc(prefab_size), 0, 1,
                                                            prefab_size})
                         .first;
        }
    }
};

} // namespace cactus::ecs
