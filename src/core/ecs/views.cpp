module;

#include <cassert>

export module cactus.core.ecs:views;

export import :world;

import std;

using size_t = std::size_t;

namespace cactus {

template <typename T, typename... Ts> struct index_of;
template <typename T, typename... Rest> struct index_of<T, T, Rest...> : std::integral_constant<size_t, 0> {};
template <typename T, typename First, typename... Rest>
struct index_of<T, First, Rest...> : std::integral_constant<size_t, 1 + index_of<T, Rest...>::value> {};

template <typename T, typename... Ts> inline constexpr size_t index_of_v = index_of<T, Ts...>::value;

template <typename T, typename... Ts> inline constexpr bool contains_v = (std::is_same_v<T, Ts> || ...);

export template <typename... Ts> struct EntityView {
    const World *world_ref;
    Entity entity;
    std::vector<void *> ptr_datas;

    explicit EntityView(const World &world_ref, const Entity &entity, std::vector<void *> &&data)
        : world_ref(&world_ref), entity(entity), ptr_datas(data) {}

    template <typename T>
        requires contains_v<T, Ts...>
    [[nodiscard]] auto get() -> T * {
        return (T *)ptr_datas[index_of_v<T, Ts...>];
    }
    template <typename T>
        requires contains_v<T, Ts...>
    [[nodiscard]] auto get() const -> const T * {
        return (const T *)ptr_datas[index_of_v<T, Ts...>];
    }

    [[nodiscard]] auto to_tuple() -> std::tuple<Entity, Ts *...> {
        return to_tuple_impl(std::make_index_sequence<sizeof...(Ts)>{});
    }

    [[nodiscard]] auto to_tuple_no_entity() -> std::tuple<Ts *...> {
        return to_tuple_no_entity_impl(std::make_index_sequence<sizeof...(Ts)>{});
    }

private:
    template <std::size_t... Is> auto to_tuple_impl(std::index_sequence<Is...>) -> std::tuple<Entity, Ts *...> {
        return std::make_tuple(entity, static_cast<Ts *>(ptr_datas[Is])...);
    }
    template <std::size_t... Is> auto to_tuple_no_entity_impl(std::index_sequence<Is...>) -> std::tuple<Entity, Ts *...> {
        return std::make_tuple(static_cast<Ts *>(ptr_datas[Is])...);
    }
};

export template <typename... Ts> struct WorldView {
    const World *world_ref;
    std::vector<size_t> archetype_list;

    explicit WorldView(const World &world_ref) : world_ref(&world_ref), archetype_list() {
        Signature signature{};
        ((signature.set(world_ref.component_atlas.get_key<Ts>().value_or(MAX_COMPONENT_COUNT - 1))),
         ...); // FIXME MAX_COMPONENT_COUNT - 1 as fallback is just a temp solution
        assert(!signature.test(MAX_COMPONENT_COUNT - 1));

        // TODO optimize, this is bruce-force
        for (const auto &[signature_key, archetype_key] : world_ref.signature_to_archetype_key_map) {
            const Signature s = world_ref.signature_atlas.get(signature_key);
            if ((s & signature) == signature) { archetype_list.push_back(archetype_key); }
        }
        archetype_list.shrink_to_fit();
    }

    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::tuple<Ts *...>;
        using difference_type = std::ptrdiff_t;

        const WorldView *source;
        size_t cur_archetype_list_index;
        size_t cur_row_index;

        EntityView<Ts...> entity_view;

        iterator(const WorldView &world_view, size_t archetype_list_index, size_t row_index)
            : source(&world_view), cur_archetype_list_index(archetype_list_index), cur_row_index(row_index),
              entity_view(world_view.world_ref, Entity{}, std::vector<void *>(sizeof...(Ts))) {
            size_t cur_archetype_key = world_view.archetype_list[cur_archetype_list_index];
        }

        auto operator*() const -> value_type { return entity_view.to_tuple(); }

        bool operator!=(const iterator &other) const {
            return cur_archetype_list_index != other.cur_archetype_list_index &&
                   cur_row_index != other.cur_row_index;
        }

        auto operator++() -> iterator & {
            // TODO
            return *this;
        }
    };

    // iterator begin() const { return iterator(*data_ref, 0); }
    // iterator end() const { return iterator(*data_ref, MAX_COMPONENT_COUNT); }
};

} // namespace cactus
