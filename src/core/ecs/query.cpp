module;

export module cactus.core.ecs:query;

import :defines;
import :table;
import cactus.common;
import cactus.core.strat;

namespace cactus {

using namespace cactus::detail::ecs;

export template <typename... Ts> struct WorldQuery {
    using world_t = World<Ts...>;

    world_t *world_ref;
    Signature signature;

    explicit WorldQuery(world_t *world_ref, Signature signature) : world_ref(world_ref), signature(signature) {}
    ~WorldQuery() = default;

    WorldQuery(const WorldQuery &other) = default;
    WorldQuery &operator=(const WorldQuery &other) = default;

    WorldQuery(WorldQuery &&other) noexcept = default;
    WorldQuery &operator=(WorldQuery &&other) noexcept = default;

    struct iterator {
        struct PrefabQuery {
            Table *table_ref;
            size_t cur_row_index;

            template <typename T>
                requires(world_t::component_register_t::template has<T>())
            [[nodiscard]] auto get() const -> std::optional<T> {
                size_t component_index = world_t::component_register_t::template get_index<T>();

                if (!table_ref->signature.test(component_index)) { return {}; }

                const void *ptr = table_ref->get_component_ptr(cur_row_index, component_index);
                return *static_cast<const T *>(ptr);
            }

            template <typename T>
                requires(world_t::component_register_t::template has<T>())
            [[nodiscard]] auto get_ptr() const -> T * {
                size_t component_index = world_t::component_register_t::template get_index<T>();

                if (!table_ref->signature.test(component_index)) { return nullptr; }

                void *ptr = table_ref->get_component_ptr(cur_row_index, component_index);
                return static_cast<T *>(ptr);
            }
            template <typename T>
                requires(world_t::component_register_t::template has<T>())
            [[nodiscard]] auto get_const_ptr() const -> const T * {
                size_t component_index = world_t::component_register_t::template get_index<T>();

                if (!table_ref->signature.test(component_index)) { return nullptr; }

                const void *ptr = table_ref->get_component_ptr(cur_row_index, component_index);
                return static_cast<const T *>(ptr);
            }
        };

        using iterator_concept = std::forward_iterator_tag;
        using value_type = std::pair<Entity, PrefabQuery>;
        using difference_type = std::ptrdiff_t;

        const WorldQuery *source = nullptr;
        size_t cur_table_index = 0;
        size_t cur_row_index = 0;

        iterator() = default;

        iterator(const WorldQuery *source, size_t table_index, size_t row_index)
            : source(source), cur_table_index(table_index), cur_row_index(row_index) {
            if (source) skip_invalid();
        }

        [[nodiscard]] auto operator*() const -> value_type {
            auto &table = source->world_ref->world_impl.tables[cur_table_index];
            return {table.owner_list_raw[cur_row_index], {.table_ref = &table, .cur_row_index = cur_row_index}};
        }

        auto operator++() -> iterator & {
            ++cur_row_index;
            skip_invalid();
            return *this;
        }

        auto operator++(int) -> iterator {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        [[nodiscard]] auto operator==(const iterator &other) const -> bool = default;

    private:
        auto skip_invalid() -> void {
            const auto &tables = source->world_ref->world_impl.tables;
            while (cur_table_index < tables.size()) {
                const auto &table = tables[cur_table_index];

                if ((table.signature & source->signature) == source->signature) {
                    if (cur_row_index < table.len) { return; }
                }

                ++cur_table_index;
                cur_row_index = 0;
            }
        }
    };

    static_assert(std::forward_iterator<iterator>, "WorldQuery::iterator must satisfy std::forward_iterator");

    [[nodiscard]] auto begin() const -> iterator { return iterator{this, 0, 0}; }

    [[nodiscard]] auto end() const -> iterator { return iterator{this, world_ref->world_impl.tables.size(), 0}; }
};

export template <typename... Ts> struct WorldQueryBuilder {
    using world_t = World<Ts...>;

    world_t *world_ref;
    Signature signature;

    explicit WorldQueryBuilder(world_t *world_ref) : world_ref(world_ref), signature(Signature{}) {}
    ~WorldQueryBuilder() = default;

    WorldQueryBuilder(const WorldQueryBuilder &other) = default;
    WorldQueryBuilder &operator=(const WorldQueryBuilder &other) = default;

    WorldQueryBuilder(WorldQueryBuilder &&other) noexcept = default;
    WorldQueryBuilder &operator=(WorldQueryBuilder &&other) noexcept = default;

    template <typename... Us>
        requires((world_t::component_register_t::template has<Us>() && ...))
    auto with() -> WorldQueryBuilder & {
        (..., signature.set(world_t::component_register_t::template get_index<Us>()));
        return *this;
    }

    [[nodiscard]] auto build() const -> WorldQuery<Ts...> { return WorldQuery<Ts...>(world_ref, signature); }
};

} // namespace cactus
