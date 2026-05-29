module;

export module cactus.core.ecs:world;

import :defines;
import :utils;
import :table;
import :query;
import cactus.common;
import cactus.core.strat;

namespace cactus {

using namespace cactus::detail::ecs;

export template <typename... Ts>
    requires(std::is_trivially_copyable_v<Ts> && ...)
struct World {
    using component_utils_t = WorldComponentUtilities<Ts...>;

    struct EntityData {
        Signature signature;
        size_t table_row_index;
    };

    SlotMap<EntityData> entities_data;

    size_t component_count;
    FixedArray<ComponentData> component_data_list;

    HashMap<Signature, size_t, SignatureHasher> signature_to_table_index_map;
    DynamicArray<Table> tables;

    [[nodiscard]] static auto make() -> World {
        size_t component_count = component_utils_t::count();
        auto component_data_list = FixedArray<ComponentData>::make(component_count);

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (..., component_data_list.set(
                      Is, {component_utils_t::template get_size<Is>(), component_utils_t::template get_align<Is>()}));
        }(std::make_index_sequence<sizeof...(Ts)>{});

        return World{.entities_data = SlotMap<EntityData>::make(),
                     .component_count = component_count,
                     .component_data_list = std::move(component_data_list),
                     .signature_to_table_index_map = HashMap<Signature, size_t, SignatureHasher>::make(),
                     .tables = DynamicArray<Table>::make()};
    }
    auto destroy() {
        for (auto &t : tables) t.destroy();

        entities_data.destroy();
        component_data_list.destroy();
        signature_to_table_index_map.destroy();
        tables.destroy();
    }
    [[nodiscard]] auto clone() = delete; // FIXME

    [[nodiscard]] auto new_entity() -> Entity { return entities_data.add(EntityData{Signature{}, 0}); }

    [[nodiscard]] auto has_entity(Entity entity) -> bool { return entities_data.has(entity); }

    [[nodiscard]] auto get_entity_signature(Entity entity) -> std::optional<Signature> {
        return entities_data.get(entity).transform([](auto data) { return data.signature; });
    }

    [[nodiscard]] auto get_component_ptr(Entity entity, size_t component_index) -> void * {
        if (component_index >= component_count) return nullptr; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr; // if entity not existed
        EntityData entity_data = entity_data_opt.value();

        Signature signature = entity_data.signature;
        _assert(!signature.any() || signature_to_table_index_map.has(signature),
                "Entity's signature should be empty or already existed");

        if (!signature.test(component_index)) return nullptr; // if entity's signature not has the component

        size_t table_index = signature_to_table_index_map.get(signature).value();
        _assert(table_index < tables.len, "Table index shoule be existed");

        return tables.get_ptr(table_index)->get_component_ptr(entity_data.table_row_index, component_index);
    }
    [[nodiscard]] auto get_component_ptr(Entity entity, size_t component_index) const -> const void * {
        if (component_index >= component_count) return nullptr; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr; // if entity not existed
        EntityData entity_data = entity_data_opt.value();

        Signature signature = entity_data.signature;
        _assert(!signature.any() || signature_to_table_index_map.has(signature),
                "Entity's signature should be empty or already existed");

        if (!signature.test(component_index)) return nullptr; // if entity's signature not has the component

        size_t table_index = signature_to_table_index_map.get(signature).value();
        _assert(table_index < tables.len, "Table index shoule be existed");

        return tables.get_ptr(table_index)->get_component_ptr(entity_data.table_row_index, component_index);
    }

    [[nodiscard]] auto has_component(Entity entity, size_t component_index) const -> bool {
        if (component_index >= component_count) return false; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return false;
        EntityData entity_data = entity_data_opt.value();

        Signature signature = entity_data.signature;
        _assert(!signature.any() || signature_to_table_index_map.has(signature),
                "Entity's signature should be empty or already existed");

        return signature.test(component_index);
    }

    auto add_component(Entity entity, size_t component_index) -> void * {
        if (component_index >= component_count) return nullptr; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr; // if entity not existed
        EntityData entity_data = entity_data_opt.value();

        Signature cur_signature = entity_data.signature;
        _assert(!cur_signature.any() || signature_to_table_index_map.has(cur_signature),
                "Entity's signature should be empty or already existed");

        // if current signature already has the component, return that component ptr
        if (cur_signature.test(component_index)) return get_component_ptr(entity, component_index);

        Signature new_signature = cur_signature;
        new_signature.set(component_index);

        // get new table, create new table if needed
        auto new_table_opt = signature_to_table_index_map.get(new_signature);
        size_t new_table_index = new_table_opt.has_value() ? new_table_opt.value() : new_table(new_signature);

        // if current signature is empty or the entity has not existed in a table: just create new row in table
        if (!cur_signature.any()) {
            Table *new_table = tables.get_ptr(new_table_index);
            new_table->new_row(entity);
            entities_data.set(entity, {new_signature, new_table->len - 1});
        }
        // else: move old data to new row, delete old row
        else {
            Table *new_table = tables.get_ptr(new_table_index);
            new_table->new_row(entity);

            size_t cur_table_index = signature_to_table_index_map.get(cur_signature).value();
            Table *cur_table = tables.get_ptr(cur_table_index);

            size_t cur_row_index = entity_data.table_row_index;

            char *cur_row_ptr = (char *)cur_table->get_row_ptr(cur_row_index);
            char *new_row_ptr = (char *)new_table->get_row_ptr(new_table->len - 1);
            for (auto cur_signature_ull = cur_signature.to_ullong(); cur_signature_ull > 0;
                 cur_signature_ull &= (cur_signature_ull - 1)) {
                size_t t_component_index = __builtin_ctzll(cur_signature_ull);
                auto t_component_data = component_data_list.get(t_component_index);

                void *src = cur_row_ptr + cur_table->get_component_offset(t_component_index);
                void *dst = new_row_ptr + new_table->get_component_offset(t_component_index);
                std::memcpy(dst, src, t_component_data->size);
            }

            if (auto moved_entity_opt = cur_table->remove_row(cur_row_index)) {
                Entity moved_entity = moved_entity_opt.value();
                _assert(entities_data.has(moved_entity), "The entity in the last of table should already existed");

                auto moved_entity_data = entities_data.get(moved_entity).value();
                moved_entity_data.table_row_index = cur_row_index;
                entities_data.set(moved_entity, moved_entity_data);
            }

            entities_data.set(entity, {new_signature, new_table->len - 1});
        }

        Table *new_table = tables.get_ptr(new_table_index);
        return new_table->get_component_ptr(new_table->len - 1, component_index);
    }
    auto add_component(Entity entity, std::initializer_list<size_t> component_index_list) -> void {
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return; // if entity not existed
        EntityData entity_data = entity_data_opt.value();

        Signature cur_signature = entity_data.signature;
        _assert(!cur_signature.any() || signature_to_table_index_map.has(cur_signature),
                "Entity's signature should be empty or already existed");

        Signature new_signature = cur_signature;
        for (size_t ci : component_index_list) {
            if (ci < component_count) new_signature.set(ci);
        }
        if (new_signature == cur_signature) return;

        // get new table, create new table if needed
        auto new_table_opt = signature_to_table_index_map.get(new_signature);
        size_t new_table_index = new_table_opt.has_value() ? new_table_opt.value() : new_table(new_signature);

        // if current signature is empty or the entity has not existed in a table: just create new row in table
        if (!cur_signature.any()) {
            Table *new_table = tables.get_ptr(new_table_index);
            new_table->new_row(entity);
            entities_data.set(entity, {new_signature, new_table->len - 1});
        }
        // else: move old data to new row, delete old row
        else {
            Table *new_table = tables.get_ptr(new_table_index);
            new_table->new_row(entity);

            size_t cur_table_index = signature_to_table_index_map.get(cur_signature).value();
            Table *cur_table = tables.get_ptr(cur_table_index);

            size_t cur_row_index = entity_data.table_row_index;

            char *cur_row_ptr = (char *)cur_table->get_row_ptr(cur_row_index);
            char *new_row_ptr = (char *)new_table->get_row_ptr(new_table->len - 1);
            for (auto cur_signature_ull = cur_signature.to_ullong(); cur_signature_ull > 0;
                 cur_signature_ull &= (cur_signature_ull - 1)) {
                size_t t_component_index = __builtin_ctzll(cur_signature_ull);
                auto t_component_data = component_data_list.get(t_component_index);

                void *src = cur_row_ptr + cur_table->get_component_offset(t_component_index);
                void *dst = new_row_ptr + new_table->get_component_offset(t_component_index);
                std::memcpy(dst, src, t_component_data->size);
            }

            if (auto moved_entity_opt = cur_table->remove_row(cur_row_index)) {
                Entity moved_entity = moved_entity_opt.value();
                _assert(entities_data.has(moved_entity), "The entity in the last of table should already existed");

                auto moved_entity_data = entities_data.get(moved_entity).value();
                moved_entity_data.table_row_index = cur_row_index;
                entities_data.set(moved_entity, moved_entity_data);
            }

            entities_data.set(entity, {new_signature, new_table->len - 1});
        }
    }

    auto remove_component(Entity entity, size_t component_index) -> bool {
        if (component_index >= component_count) return false; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return false; // if entity not existed
        EntityData entity_data = entity_data_opt.value();

        Signature cur_signature = entity_data.signature;
        _assert(!cur_signature.any() || signature_to_table_index_map.has(cur_signature),
                "Entity's signature should be empty or already existed");

        if (!cur_signature.test(component_index)) return false;

        Signature new_signature = cur_signature;
        new_signature.reset(component_index);

        if (!new_signature.any()) {
            size_t cur_table_index = signature_to_table_index_map.get(cur_signature).value();
            Table *cur_table = tables.get_ptr(cur_table_index);

            size_t cur_row_index = entity_data.table_row_index;

            if (auto moved_entity_opt = cur_table->remove_row(cur_row_index)) {
                Entity moved_entity = moved_entity_opt.value();
                _assert(entities_data.has(moved_entity), "The entity in the last of table should already existed");

                auto moved_entity_data = entities_data.get(moved_entity).value();
                moved_entity_data.table_row_index = cur_row_index;
                entities_data.set(moved_entity, moved_entity_data);
            }

            entities_data.set(entity, {new_signature, 0});
            return true;
        }

        // get new table, create new table if needed
        auto new_table_opt = signature_to_table_index_map.get(new_signature);
        size_t new_table_index = new_table_opt.has_value() ? new_table_opt.value() : new_table(new_signature);

        Table *new_table = tables.get_ptr(new_table_index);
        new_table->new_row(entity);

        size_t cur_table_index = signature_to_table_index_map.get(cur_signature).value();
        Table *cur_table = tables.get_ptr(cur_table_index);

        size_t cur_row_index = entity_data.table_row_index;

        char *cur_row_ptr = (char *)cur_table->get_row_ptr(cur_row_index);
        char *new_row_ptr = (char *)new_table->get_row_ptr(new_table->len - 1);
        for (auto new_signature_ull = new_signature.to_ullong(); new_signature_ull > 0;
             new_signature_ull &= (new_signature_ull - 1)) {
            size_t t_component_index = __builtin_ctzll(new_signature_ull);
            auto t_component_data = component_data_list.get(t_component_index);

            void *src = cur_row_ptr + cur_table->get_component_offset(t_component_index);
            void *dst = new_row_ptr + new_table->get_component_offset(t_component_index);
            std::memcpy(dst, src, t_component_data->size);
        }

        if (auto moved_entity_opt = cur_table->remove_row(cur_row_index)) {
            Entity moved_entity = moved_entity_opt.value();
            _assert(entities_data.has(moved_entity), "The entity in the last of table should already existed");

            auto moved_entity_data = entities_data.get(moved_entity).value();
            moved_entity_data.table_row_index = cur_row_index;
            entities_data.set(moved_entity, moved_entity_data);
        }

        entities_data.set(entity, {new_signature, new_table->len - 1});

        return true;
    }
    auto remove_component(Entity entity, std::initializer_list<size_t> component_index_list) -> void {
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return; // if entity not existed
        EntityData entity_data = entity_data_opt.value();

        Signature cur_signature = entity_data.signature;
        _assert(!cur_signature.any() || signature_to_table_index_map.has(cur_signature),
                "Entity's signature should be empty or already existed");

        Signature new_signature = cur_signature;
        for (size_t ci : component_index_list) {
            if (ci < component_count) new_signature.reset(ci);
        }

        if (!new_signature.any()) {
            size_t cur_table_index = signature_to_table_index_map.get(cur_signature).value();
            Table *cur_table = tables.get_ptr(cur_table_index);

            size_t cur_row_index = entity_data.table_row_index;

            if (auto moved_entity_opt = cur_table->remove_row(cur_row_index)) {
                Entity moved_entity = moved_entity_opt.value();
                _assert(entities_data.has(moved_entity), "The entity in the last of table should already existed");

                auto moved_entity_data = entities_data.get(moved_entity).value();
                moved_entity_data.table_row_index = cur_row_index;
                entities_data.set(moved_entity, moved_entity_data);
            }

            entities_data.set(entity, {new_signature, 0});
        }

        // get new table, create new table if needed
        auto new_table_opt = signature_to_table_index_map.get(new_signature);
        size_t new_table_index = new_table_opt.has_value() ? new_table_opt.value() : new_table(new_signature);

        Table *new_table = tables.get_ptr(new_table_index);
        new_table->new_row(entity);

        size_t cur_table_index = signature_to_table_index_map.get(cur_signature).value();
        Table *cur_table = tables.get_ptr(cur_table_index);

        size_t cur_row_index = entity_data.table_row_index;

        char *cur_row_ptr = (char *)cur_table->get_row_ptr(cur_row_index);
        char *new_row_ptr = (char *)new_table->get_row_ptr(new_table->len - 1);
        for (auto new_signature_ull = new_signature.to_ullong(); new_signature_ull > 0;
             new_signature_ull &= (new_signature_ull - 1)) {
            size_t t_component_index = __builtin_ctzll(new_signature_ull);
            auto t_component_data = component_data_list.get(t_component_index);

            void *src = cur_row_ptr + cur_table->get_component_offset(t_component_index);
            void *dst = new_row_ptr + new_table->get_component_offset(t_component_index);
            std::memcpy(dst, src, t_component_data->size);
        }

        if (auto moved_entity_opt = cur_table->remove_row(cur_row_index)) {
            Entity moved_entity = moved_entity_opt.value();
            _assert(entities_data.has(moved_entity), "The entity in the last of table should already existed");

            auto moved_entity_data = entities_data.get(moved_entity).value();
            moved_entity_data.table_row_index = cur_row_index;
            entities_data.set(moved_entity, moved_entity_data);
        }

        entities_data.set(entity, {new_signature, new_table->len - 1});
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    [[nodiscard]] auto get_component(Entity entity) const -> std::optional<T> {
        const void *ptr = get_component_ptr(entity, component_utils_t::template get_index<T>());
        if (ptr == nullptr) return {};
        return *(T *)ptr;
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    [[nodiscard]] auto get_component_ptr(Entity entity) -> T * {
        return (T *)get_component_ptr(entity, component_utils_t::template get_index<T>());
    }
    template <typename T>
        requires(component_utils_t::template has<T>())
    [[nodiscard]] auto get_component_ptr(Entity entity) const -> const T * {
        return (const T *)get_component_ptr(entity, component_utils_t::template get_index<T>());
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    [[nodiscard]] auto has_component(Entity entity) const -> bool {
        return has_component(entity, component_utils_t::template get_index<T>());
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    auto add_component(Entity entity) -> T * {
        return (T *)add_component(entity, component_utils_t::template get_index<T>());
    }
    template <typename... Us>
        requires(sizeof...(Us) > 1 && (component_utils_t::template has<Us>() && ...))
    auto add_component(Entity entity) -> void {
        add_component(entity, {component_utils_t::template get_index<Us>()...});
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    auto remove_component(Entity entity) -> bool {
        return remove_component(entity, component_utils_t::template get_index<T>());
    }
    template <typename... Us>
        requires(sizeof...(Us) > 1 && (component_utils_t::template has<Us>() && ...))
    auto remove_component(Entity entity) -> void {
        remove_component(entity, {component_utils_t::template get_index<Us>()...});
    }

    [[nodiscard]] auto query_builder() -> WorldQueryBuilder<Ts...> { return WorldQueryBuilder<Ts...>(this); }

private:
    auto new_table(Signature signature) -> size_t {
        _assert(!signature_to_table_index_map.has(signature), "Signature should not already existed");

        tables.append(Table::make(signature, component_data_list));
        signature_to_table_index_map.add(signature, tables.len - 1);

        return tables.len - 1;
    }
};

} // namespace cactus
