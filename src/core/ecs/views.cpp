module;

#include <cassert>

export module cactus.core.ecs:views;

export import :world;
export import :signature;

import std;

using size_t = std::size_t;

namespace cactus {

template <typename T, typename... Ts> struct index_of;
template <typename T, typename... Rest> struct index_of<T, T, Rest...> : std::integral_constant<size_t, 0> {};
template <typename T, typename First, typename... Rest>
struct index_of<T, First, Rest...> : std::integral_constant<size_t, 1 + index_of<T, Rest...>::value> {};

template <typename T, typename... Ts> inline constexpr size_t index_of_v = index_of<T, Ts...>::value;

template <typename T, typename... Ts> inline constexpr bool contains_v = (std::is_same_v<T, Ts> || ...);

export template <typename... Ts> struct EntityRowDataView {
    const World *world_ref;
    size_t archetype_index;
    size_t row_index;

    Entity entity;
    const void *row_ptr;
    std::vector<size_t> row_ptr_offsets;

    EntityRowDataView(const World &world_ref, size_t archetype_index, size_t row_index)
        : world_ref(&world_ref), archetype_index(archetype_index), row_index(row_index) {

        entity = world_ref.archetypes[archetype_index].get_owner(row_index);

        row_ptr = world_ref.archetypes[archetype_index].get_row_ptr(row_index);

        row_ptr_offsets.reserve(sizeof...(Ts));
        auto find_type_ptr_in_archetype = [&](auto type_tag) -> void * {
            using T = typename decltype(type_tag)::type;
            auto component_key_opt = world_ref.component_registry.get_key<T>();

            assert(component_key_opt.has_value());
            if (!component_key_opt.has_value()) {
                // TODO error
            }

            return world_ref.archetypes[archetype_index].get_component_row_offset(component_key_opt.value());
        };
        (..., row_ptr_offsets.push_back(find_type_ptr_in_archetype(std::type_identity<Ts>{})));
    }

    auto change_to_next_row() -> bool {
        if (row_index == world_ref->archetypes[archetype_index].len - 1) return false;

        ++row_index;

        entity = world_ref->archetypes[archetype_index].get_owner(row_index);
        row_ptr = (void *)((char *)row_ptr + world_ref->archetypes[archetype_index].row_size);

        return true;
    }

    auto change_archetype(size_t new_archetype_index, size_t new_row_index = 0) {
        archetype_index = new_archetype_index;
        row_index = new_row_index;

        entity = world_ref->archetypes[archetype_index].get_owner(row_index);

        row_ptr = world_ref->archetypes[archetype_index].get_row_ptr(row_index);

        row_ptr_offsets.reserve(sizeof...(Ts));
        auto find_type_ptr_in_archetype = [&](auto type_tag) -> void * {
            using T = typename decltype(type_tag)::type;
            auto component_key_opt = world_ref->component_registry.get_key<T>();

            assert(component_key_opt.has_value());
            if (!component_key_opt.has_value()) {
                // TODO error
            }

            return world_ref->archetypes[archetype_index].get_component_row_offset(component_key_opt.value());
        };
        (..., row_ptr_offsets.push_back(find_type_ptr_in_archetype(std::type_identity<Ts>{})));
    }

    template <typename T>
        requires contains_v<T, Ts...>
    [[nodiscard]] auto get() -> T * {
        return (T *)((char *)row_ptr + row_ptr_offsets[index_of_v<T, Ts...>]);
    }
    template <typename T>
        requires contains_v<T, Ts...>
    [[nodiscard]] auto get() const -> const T * {
        return (const T *)((char *)row_ptr + row_ptr_offsets[index_of_v<T, Ts...>]);
    }

    [[nodiscard]] auto to_tuple() -> std::tuple<Entity, Ts *...> {
        return to_tuple_impl(std::make_index_sequence<sizeof...(Ts)>{});
    }

    [[nodiscard]] auto to_tuple_no_entity() -> std::tuple<Ts *...> {
        return to_tuple_no_entity_impl(std::make_index_sequence<sizeof...(Ts)>{});
    }

private:
    template <std::size_t... Is> auto to_tuple_impl(std::index_sequence<Is...>) -> std::tuple<Entity, Ts *...> {
        return std::make_tuple(entity, (Ts *)((char *)row_ptr + row_ptr_offsets[Is])...);
    }
    template <std::size_t... Is> auto to_tuple_no_entity_impl(std::index_sequence<Is...>) -> std::tuple<Entity, Ts *...> {
        return std::make_tuple((Ts *)((char *)row_ptr + row_ptr_offsets[Is])...);
    }
};

export template <typename... Ts> struct WorldView {
    const World *world_ref;
    SignatureAtlasKey signature_key;

    std::vector<size_t> archetype_index_list;

    WorldView(const World &world_ref) : world_ref(&world_ref), signature_key(EMPTY_SIGNATURE_KEY) {
        // construct signature from types
        auto add_type_to_signature = [&](auto type_tag) {
            using T = typename decltype(type_tag)::type;
            auto component_key_opt = world_ref.component_registry.get_key<T>();

            assert(component_key_opt.has_value());
            if (!component_key_opt.has_value()) {
                // TODO error
            }

            signature_key = world_ref.signature_atlas.get_by_add(signature_key, component_key_opt.value());
        };
        (..., add_type_to_signature(std::type_identity<Ts>{}));

        // TODO optimize, this is bruce-force
        for (const auto &[signature_key, archetype_key] : world_ref.signature_to_archetype_key_map) {
            if (world_ref.signature_atlas.signature_contains(signature_key, signature_key)) {
                archetype_index_list.push_back(archetype_key);
            }
        }
    }

    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::tuple<Entity, Ts *...>;
        using difference_type = std::ptrdiff_t;

        const WorldView *source;

        size_t cur_archetype_index_list_index;
        size_t cur_row_index;

        EntityRowDataView<Ts...> entity_view;

        iterator(const WorldView &source, size_t archetype_index_list_index, size_t row_index)
            : source(&source), cur_archetype_index_list_index(archetype_index_list_index), cur_row_index(row_index) {
            assert(archetype_index_list_index < source.archetype_index_list.size());
            size_t archetype_index = source.archetype_index_list[archetype_index_list_index];
            assert(archetype_index < source.world_ref->archetypes.size());

            entity_view = EntityRowDataView<Ts...>(source.world_ref, archetype_index, row_index);
        }

        auto operator*() const -> value_type { return entity_view.to_tuple(); }

        bool operator!=(const iterator &other) const {
            return cur_archetype_index_list_index != other.cur_archetype_index_list_index &&
                   cur_row_index != other.cur_row_index;
        }

        auto operator++() -> iterator & {
            if (entity_view.change_to_next_row()) {
                ++cur_row_index;
            } else {
                ++cur_archetype_index_list_index;
                cur_row_index = 0;

                assert(cur_archetype_index_list_index < source->archetype_index_list.size());
                size_t archetype_index = source->archetype_index_list[cur_archetype_index_list_index];
                assert(archetype_index < source->world_ref->archetypes.size());

                entity_view.change_archetype(archetype_index);
            }

            return *this;
        }
    };

    iterator begin() const { return iterator(*this, 0, 0); }
    iterator end() const { return iterator(*this, archetype_index_list.size(), 0); }
};

} // namespace cactus
