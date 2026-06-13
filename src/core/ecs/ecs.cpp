module;

#include <climits>

export module cact.core.ecs;

import cact.core.strat;
export import :utils;

using namespace cact::detail::ecs;

export namespace cact {

constexpr size_t MAX_WORLD_COMPONENTS_COUNT = 32;

// DEFINES
// ============================================================================
struct Entity {
    size_t index : (sizeof(size_t) * CHAR_BIT) - CHAR_BIT;
    size_t gen : CHAR_BIT;

    operator size_t() const noexcept { return *(const size_t *)this; }
};

using Signature = std::bitset<MAX_WORLD_COMPONENTS_COUNT>;
struct SignatureHasher {
    size_t operator()(const Signature &s) const noexcept {
        return static_cast<size_t>(s.to_ullong());
    }
};

struct ComponentData {
    size_t size, align;
};

// FORWARD DECLARE
// ============================================================================
template <typename... Ts>
requires(std::is_trivially_copyable_v<Ts> && ...)
struct World;

struct Table;

template <typename... Ts> struct WorldQuery;

template <typename... Ts> struct WorldQueryBuilder;

// TABLE
// ============================================================================
struct Table {
    char *table_raw = nullptr;
    Entity *owner_list_raw = nullptr;
    size_t row_size;
    size_t len = 0;
    size_t cap = 0;

    Signature signature;

    FixedArray<size_t> component_offset_list;

    // ========================================================================
    [[nodiscard]] static Table make(Signature signature,
    const FixedArray<ComponentData> &component_data_list) noexcept {
        size_t offset = 0;
        size_t max_align = 1;

        auto component_offset_list = FixedArray<size_t>::make(component_data_list.len);
        for (auto signature_ull = signature.to_ullong();
             signature_ull > 0;
             signature_ull &= (signature_ull - 1)) {
            int component_index = __builtin_ctzll(signature_ull);
            auto component_data_opt = component_data_list.get(component_index);
            _assert(component_data_opt.has_value(), "Signature's component should be existed in component list");

            offset = align_up(offset, component_data_opt->align);

            component_offset_list.set(component_index, offset);

            offset += component_data_opt->size;

            max_align = std::max(max_align, component_data_opt->align);
        }
        size_t row_size = align_up(offset, max_align);

        return Table{.table_raw = nullptr,
                     .owner_list_raw = nullptr,
                     .row_size = row_size,
                     .len = 0,
                     .cap = 0,
                     .signature = signature,
                     .component_offset_list = std::move(component_offset_list)};
    }

    void destroy() noexcept {
        if (table_raw) std::free(table_raw);
        if (owner_list_raw) std::free(owner_list_raw);
        component_offset_list.destroy();
    }

    [[nodiscard]] Table clone() noexcept = delete; // TODO

    // ========================================================================
    [[nodiscard]] const void * get_row_ptr(size_t row_index) const noexcept {
        _assert(row_index < len, "Row index out of bounds");
        return table_raw + row_index * row_size;
    }
    [[nodiscard]] void * get_row_ptr(size_t row_index) noexcept {
        _assert(row_index < len, "Row index out of bounds");
        return table_raw + row_index * row_size;
    }

    [[nodiscard]] size_t get_component_offset(size_t component_index) const noexcept {
        _assert(component_index < component_offset_list.len, "Component index out of bounds");
        return component_offset_list.get(component_index).value();
    }

    [[nodiscard]] const void * get_component_ptr(size_t row_index,
    size_t component_index) const noexcept {
        return (const char *)get_row_ptr(row_index) + get_component_offset(component_index);
    }
    [[nodiscard]] void * get_component_ptr(size_t row_index,
    size_t component_index) noexcept {
        return (char *)get_row_ptr(row_index) + get_component_offset(component_index);
    }

    void reserve(size_t new_cap) noexcept {
        if (new_cap <= cap) return;

        auto *new_table_raw = (char *)std::realloc(table_raw, new_cap * row_size);
        _assert(new_table_raw, "Failed to reallocate table_raw");

        auto *new_owner_list_raw =
            (Entity *)std::realloc(owner_list_raw, new_cap * sizeof(Entity));
        _assert(new_owner_list_raw, "Failed to reallocate owner_list_raw");

        table_raw = new_table_raw;
        owner_list_raw = new_owner_list_raw;
        cap = new_cap;
    }

    size_t new_row(Entity entity_owner) noexcept {
        if (len >= cap) {
            size_t new_cap = cap < 4 ? 4 : cap + (cap / 2);
            reserve(new_cap);
        }

        ++len;

        char *last_row_ptr = table_raw + (len - 1) * row_size;
        std::memset(last_row_ptr, 0, row_size);

        owner_list_raw[len - 1] = entity_owner;

        return len - 1;
    }

    // NOTE: this return the entity owner of the last row if it require to be moved by this
    std::optional<Entity> remove_row(size_t index) noexcept {
        if (index >= len) return {};

        if (index != len - 1) {
            Entity moved_entity = owner_list_raw[len - 1];

            char *t_row_ptr = table_raw + index * row_size;
            char *last_row_ptr = table_raw + (len - 1) * row_size;
            std::memcpy(t_row_ptr, last_row_ptr, row_size);

            owner_list_raw[index] = owner_list_raw[len - 1];

            --len;

            return moved_entity;
        }

        --len;

        return {};
    }
};

// WORLD
// ============================================================================
template <typename... Ts>
requires(std::is_trivially_copyable_v<Ts> && ...)
struct World {
    using component_utils_t = WorldComponentUtilities<Ts...>;

    struct EntityData {
        Signature signature;
        size_t table_row_index;
    };

    // ========================================================================
    SlotMap<EntityData> entities_data;

    size_t component_count;
    FixedArray<ComponentData> component_data_list;

    HashMap<Signature, size_t, SignatureHasher> signature_to_table_index_map;
    DynamicArray<Table> tables;

    // ========================================================================
    [[nodiscard]] static World make() noexcept {
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
    void destroy() noexcept {
        for (auto &t : tables) t.destroy();

        entities_data.destroy();
        component_data_list.destroy();
        signature_to_table_index_map.destroy();
        tables.destroy();
    }

    [[nodiscard]] World clone() noexcept = delete; // FIXME

    // ========================================================================
    [[nodiscard]] Entity new_entity() noexcept {
        return entities_data.add(EntityData{Signature{}, 0});
    }

    [[nodiscard]] bool has_entity(Entity entity) noexcept {
        return entities_data.has(entity);
    }

    [[nodiscard]]
    std::optional<Signature> get_entity_signature(Entity entity) noexcept {
        return entities_data
               .get(entity)
               .transform([](auto data) { return data.signature; });
    }

    [[nodiscard]]
    void * get_component_ptr(Entity entity, size_t component_index) noexcept {
        if (component_index >= component_count) return nullptr;
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr;
        EntityData entity_data = entity_data_opt.value();

        Signature signature = entity_data.signature;
        _assert(!signature.any() || signature_to_table_index_map.has(signature),
                "Entity's signature should be empty or already existed");

        if (!signature.test(component_index))
            return nullptr; // if entity's signature not has the component

        size_t table_index = signature_to_table_index_map.get(signature).value();
        _assert(table_index < tables.len, "Table index shoule be existed");

        return tables.get_ptr(table_index)
               ->get_component_ptr(entity_data.table_row_index, component_index);
    }

    [[nodiscard]]
    const void * get_component_ptr(Entity entity, size_t component_index) const noexcept {
        if (component_index >= component_count) return nullptr;
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr;
        EntityData entity_data = entity_data_opt.value();

        Signature signature = entity_data.signature;
        _assert(!signature.any() || signature_to_table_index_map.has(signature),
                "Entity's signature should be empty or already existed");

        if (!signature.test(component_index))
            return nullptr; // if entity's signature not has the component

        size_t table_index = signature_to_table_index_map.get(signature).value();
        _assert(table_index < tables.len, "Table index shoule be existed");

        return tables.get_ptr(table_index)->get_component_ptr(entity_data.table_row_index, component_index);
    }

    [[nodiscard]]
    bool has_component(Entity entity, size_t component_index) const noexcept {
        if (component_index >= component_count) return false;
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return false;
        EntityData entity_data = entity_data_opt.value();

        Signature signature = entity_data.signature;
        _assert(!signature.any() || signature_to_table_index_map.has(signature),
                "Entity's signature should be empty or already existed");

        return signature.test(component_index);
    }

    void * add_component(Entity entity, size_t component_index) noexcept {
        if (component_index >= component_count) return nullptr;
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr;
        EntityData entity_data = entity_data_opt.value();

        Signature cur_signature = entity_data.signature;
        _assert(!cur_signature.any() || signature_to_table_index_map.has(cur_signature),
            "Entity's signature should be empty or already existed");

        // if current signature already has the component, return that component ptr
        if (cur_signature.test(component_index))
            return get_component_ptr(entity, component_index);

        Signature new_signature = cur_signature;
        new_signature.set(component_index);

        // get new table, create new table if needed
        auto new_table_opt = signature_to_table_index_map.get(new_signature);
        size_t new_table_index =
            new_table_opt.has_value() ? new_table_opt.value() : new_table(new_signature);

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

            size_t cur_table_index =
                signature_to_table_index_map.get(cur_signature).value();
            Table *cur_table = tables.get_ptr(cur_table_index);

            size_t cur_row_index = entity_data.table_row_index;

            char *cur_row_ptr = (char *)cur_table->get_row_ptr(cur_row_index);
            char *new_row_ptr = (char *)new_table->get_row_ptr(new_table->len - 1);
            for (auto cur_signature_ull = cur_signature.to_ullong(); cur_signature_ull > 0;
                 cur_signature_ull &= (cur_signature_ull - 1)) {
                size_t t_component_index = __builtin_ctzll(cur_signature_ull);
                auto t_component_data = component_data_list.get(t_component_index);

                void *src =
                    cur_row_ptr + cur_table->get_component_offset(t_component_index);
                void *dst =
                    new_row_ptr + new_table->get_component_offset(t_component_index);
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
    void add_component(Entity entity, std::initializer_list<size_t> component_index_list) noexcept {
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
        size_t new_table_index =
            new_table_opt.has_value() ? new_table_opt.value() : new_table(new_signature);

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

            size_t cur_table_index =
                signature_to_table_index_map.get(cur_signature).value();
            Table *cur_table = tables.get_ptr(cur_table_index);

            size_t cur_row_index = entity_data.table_row_index;

            char *cur_row_ptr = (char *)cur_table->get_row_ptr(cur_row_index);
            char *new_row_ptr = (char *)new_table->get_row_ptr(new_table->len - 1);
            for (auto cur_signature_ull = cur_signature.to_ullong(); cur_signature_ull > 0;
                 cur_signature_ull &= (cur_signature_ull - 1)) {
                size_t t_component_index = __builtin_ctzll(cur_signature_ull);
                auto t_component_data = component_data_list.get(t_component_index);

                void *src =
                    cur_row_ptr + cur_table->get_component_offset(t_component_index);
                void *dst =
                    new_row_ptr + new_table->get_component_offset(t_component_index);
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

    bool remove_component(Entity entity, size_t component_index) noexcept {
        if (component_index >= component_count) return false;
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return false;
        EntityData entity_data = entity_data_opt.value();

        Signature cur_signature = entity_data.signature;
        _assert(!cur_signature.any() || signature_to_table_index_map.has(cur_signature),
            "Entity's signature should be empty or already existed");

        if (!cur_signature.test(component_index)) return false;

        Signature new_signature = cur_signature;
        new_signature.reset(component_index);

        if (!new_signature.any()) {
            size_t cur_table_index =
                signature_to_table_index_map.get(cur_signature).value();
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
        size_t new_table_index =
            new_table_opt.has_value() ? new_table_opt.value() : new_table(new_signature);

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

    void remove_component(Entity entity, std::initializer_list<size_t> component_index_list) noexcept {
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
            size_t cur_table_index =
                signature_to_table_index_map.get(cur_signature).value();
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
        size_t new_table_index =
            new_table_opt.has_value() ? new_table_opt.value() : new_table(new_signature);

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
    [[nodiscard]] std::optional<T> get_component(Entity entity) const noexcept {
        const void *ptr =
            get_component_ptr(entity, component_utils_t::template get_index<T>());
        if (ptr == nullptr) return {};
        return *(T *)ptr;
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    [[nodiscard]] T * get_component_ptr(Entity entity) noexcept {
        return (T *)get_component_ptr(entity, component_utils_t::template get_index<T>());
    }
    template <typename T>
        requires(component_utils_t::template has<T>())
    [[nodiscard]] const T * get_component_ptr(Entity entity) const noexcept {
        return (const T *)get_component_ptr(entity, component_utils_t::template get_index<T>());
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    [[nodiscard]] bool has_component(Entity entity) const noexcept {
        return has_component(entity, component_utils_t::template get_index<T>());
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    T * add_component(Entity entity) noexcept {
        return (T *)add_component(entity, component_utils_t::template get_index<T>());
    }
    template <typename... Us>
        requires(sizeof...(Us) > 1 && (component_utils_t::template has<Us>() && ...))
    void add_component(Entity entity) noexcept {
        add_component(entity, {component_utils_t::template get_index<Us>()...});
    }

    template <typename T>
        requires(component_utils_t::template has<T>())
    bool remove_component(Entity entity) noexcept {
        return remove_component(entity, component_utils_t::template get_index<T>());
    }
    template <typename... Us>
        requires(sizeof...(Us) > 1 && (component_utils_t::template has<Us>() && ...))
    void remove_component(Entity entity) noexcept {
        remove_component(entity, {component_utils_t::template get_index<Us>()...});
    }

    [[nodiscard]] WorldQueryBuilder<Ts...> query_builder() noexcept {
        return WorldQueryBuilder<Ts...>(this);
    }

private:
    size_t new_table(Signature signature) noexcept {
        _assert(!signature_to_table_index_map.has(signature), "Signature should not already existed");

        tables.append(Table::make(signature, component_data_list));
        signature_to_table_index_map.add(signature, tables.len - 1);

        return tables.len - 1;
    }
};

// QUERY
// ============================================================================
template <typename... Ts>
struct WorldQuery {
    using world_t = World<Ts...>;

    // ========================================================================
    world_t *world_ref;
    Signature signature;

    // ========================================================================
    explicit WorldQuery(world_t *world_ref, Signature signature)
        : world_ref(world_ref)
        , signature(signature) {}
    ~WorldQuery() = default;

    WorldQuery(const WorldQuery &other) = default;
    WorldQuery &operator=(const WorldQuery &other) = default;

    WorldQuery(WorldQuery &&other) noexcept = default;
    WorldQuery &operator=(WorldQuery &&other) noexcept = default;

    // ITERATOR
    // ========================================================================
    struct iterator {
        struct PrefabQuery {
            Table *table_ref;
            size_t cur_row_index;

            template <typename T>
            requires(world_t::component_utils_t::template has<T>())
            [[nodiscard]] std::optional<T> get() const {
                size_t component_index =
                    world_t::component_utils_t::template get_index<T>();

                if (!table_ref->signature.test(component_index)) { return {}; }

                const void *ptr =
                    table_ref->get_component_ptr(cur_row_index, component_index);
                return *static_cast<const T *>(ptr);
            }

            template <typename T>
            requires(world_t::component_utils_t::template has<T>())
            [[nodiscard]] T * get_ptr() const {
                size_t component_index =
                    world_t::component_utils_t::template get_index<T>();

                if (!table_ref->signature.test(component_index)) { return nullptr; }

                void *ptr =
                    table_ref->get_component_ptr(cur_row_index, component_index);
                return static_cast<T *>(ptr);
            }
            template <typename T>
            requires(world_t::component_utils_t::template has<T>())
            [[nodiscard]] const T * get_const_ptr() const {
                size_t component_index =
                    world_t::component_utils_t::template get_index<T>();

                if (!table_ref->signature.test(component_index)) { return nullptr; }

                const void *ptr =
                    table_ref->get_component_ptr(cur_row_index, component_index);
                return static_cast<const T *>(ptr);
            }
        };

        using iterator_concept = std::forward_iterator_tag;
        using value_type = std::pair<Entity, PrefabQuery>;
        using difference_type = std::ptrdiff_t;

        // ====================================================================
        const WorldQuery *source = nullptr;
        size_t cur_table_index = 0;
        size_t cur_row_index = 0;

        // ====================================================================
        iterator() = default;
        iterator(const WorldQuery *source, size_t table_index, size_t row_index)
            : source(source), cur_table_index(table_index), cur_row_index(row_index) {
            if (source) skip_invalid();
        }

        [[nodiscard]] value_type operator*() const {
            auto *table = source->world_ref->tables.get_ptr(cur_table_index);
            return {
                table->owner_list_raw[cur_row_index], 
                PrefabQuery{.table_ref = table, .cur_row_index = cur_row_index}};
        }

        iterator & operator++() {
            ++cur_row_index;
            skip_invalid();
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        [[nodiscard]] bool operator==(const iterator &other) const = default;

    private:
        void skip_invalid() {
            const auto &tables = source->world_ref->tables;
            while (cur_table_index < tables.len) {
                const auto *table = tables.get_ptr(cur_table_index);

                if ((table->signature & source->signature) == source->signature) {
                    if (cur_row_index < table->len) { return; }
                }

                ++cur_table_index;
                cur_row_index = 0;
            }
        }
    };

    static_assert(std::forward_iterator<iterator>, "WorldQuery::iterator must satisfy std::forward_iterator");

    [[nodiscard]] iterator begin() const { return iterator{this, 0, 0}; }
    [[nodiscard]] iterator end() const { return iterator{this, world_ref->tables.len, 0}; }
};

template <typename... Ts> 
struct WorldQueryBuilder {
    using world_t = World<Ts...>;

    // ========================================================================
    world_t *world_ref;
    Signature signature;

    // ========================================================================
    explicit WorldQueryBuilder(world_t *world_ref) 
        : world_ref(world_ref)
        , signature(Signature{}) {}
    ~WorldQueryBuilder() = default;

    WorldQueryBuilder(const WorldQueryBuilder &other) = default;
    WorldQueryBuilder &operator=(const WorldQueryBuilder &other) = default;

    WorldQueryBuilder(WorldQueryBuilder &&other) noexcept = default;
    WorldQueryBuilder &operator=(WorldQueryBuilder &&other) noexcept = default;

    // ========================================================================
    template <typename... Us>
    requires((world_t::component_utils_t::template has<Us>() && ...))
    WorldQueryBuilder & with() {
        (..., signature.set(world_t::component_utils_t::template get_index<Us>()));
        return *this;
    }

    [[nodiscard]] WorldQuery<Ts...> build() const { 
        return WorldQuery<Ts...>(world_ref, signature); 
    }
};

} // namespace cact
