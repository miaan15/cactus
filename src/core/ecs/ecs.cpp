module;

#include <cassert>

export module cactus.core.ecs;

import std;
import cactus.core.strat;

using size_t = std::size_t;

namespace cactus {

[[nodiscard]] static constexpr auto align_up(size_t offset, size_t align) -> size_t {
    return (offset + align - 1) & ~(align - 1);
}

export constexpr size_t MAX_WORLD_COMPONENTS_COUNT = 32;

export template <typename... Ts>
    requires(sizeof...(Ts) <= MAX_WORLD_COMPONENTS_COUNT && sizeof...(Ts) > 0)
struct WorldComponentsRegister {
    [[nodiscard]] static constexpr auto size() -> size_t { return sizeof...(Ts); }

    template <typename T> [[nodiscard]] static constexpr auto has() -> bool { return (std::is_same_v<T, Ts> || ...); }
    template <typename T> [[nodiscard]] static constexpr auto get_index() -> size_t {
        size_t i = 0, res = 0;
        (void)((std::is_same_v<T, Ts> ? (res = i, false) : (++i, true)) && ...);
        return res;
    }

    template <size_t I> using component_at_t = typename std::tuple_element<I, std::tuple<Ts...>>::type;

    template <size_t I> [[nodiscard]] constexpr auto get_size() -> size_t {
        static_assert(I < sizeof...(Ts), "Component index out of bounds");
        return sizeof(component_at_t<I>);
    }
    template <size_t I> [[nodiscard]] constexpr auto get_align() -> size_t {
        static_assert(I < sizeof...(Ts), "Component index out of bounds");
        return alignof(component_at_t<I>);
    }

    [[nodiscard]] constexpr auto get_total_size() -> size_t {
        size_t res = 0;
        size_t max_align = 1;
        (..., (max_align = std::max(max_align, alignof(Ts)), res = align_up(res, alignof(Ts)) + sizeof(Ts)));
        return align_up(res, max_align);
    }
};

export using Entity = SlotMapKey;
export using Signature = std::bitset<MAX_WORLD_COMPONENTS_COUNT>;
struct SignatureHasher {
    auto operator()(const Signature &s) const -> size_t { return static_cast<size_t>(s.to_ullong()); }
};

struct ComponentData {
    size_t size, align;
};

struct WorldTable {
    char *table_raw;
    Entity *owner_list_raw;
    size_t row_size;
    size_t len;
    size_t cap;

    Signature signature;

    FixedArr<size_t> component_offset_list;

    explicit WorldTable(Signature signature, size_t component_count, const FixedArr<ComponentData> &component_data_list)
        : table_raw(nullptr), owner_list_raw(nullptr), len(0), cap(0), signature(signature),
          component_offset_list(component_count) {
        size_t offset = 0;
        size_t max_align = 1;

        auto signature_ull = signature.to_ullong();
        for (auto signature_ull = signature.to_ullong(); signature_ull > 0; signature_ull &= (signature_ull - 1)) {
            int component_index = __builtin_ctzll(signature_ull);
            auto component_data_opt = component_data_list.get(component_index);
            assert(component_data_opt.has_value());

            offset = align_up(offset, component_data_opt->align);

            component_offset_list.set(component_index, offset);

            offset += component_data_opt->size;

            max_align = std::max(max_align, component_data_opt->align);
        }
        row_size = align_up(offset, max_align);
    }

    ~WorldTable() {
        if (table_raw != nullptr) std::free(table_raw);
        if (owner_list_raw != nullptr) std::free(owner_list_raw);
    }

    WorldTable(const WorldTable &other) = delete;
    WorldTable &operator=(const WorldTable &other) = delete;

    WorldTable(WorldTable &&other) noexcept
        : table_raw(other.table_raw), owner_list_raw(other.owner_list_raw), row_size(other.row_size), len(other.len),
          cap(other.cap), signature(std::move(other.signature)), component_offset_list(std::move(other.component_offset_list)) {
        other.table_raw = nullptr;
        other.owner_list_raw = nullptr;
        other.row_size = 0;
        other.len = 0;
        other.cap = 0;
    }

    WorldTable &operator=(WorldTable &&other) noexcept {
        if (this != &other) {
            std::swap(table_raw, other.table_raw);
            std::swap(owner_list_raw, other.owner_list_raw);
            std::swap(row_size, other.row_size);
            std::swap(len, other.len);
            std::swap(cap, other.cap);
            std::swap(signature, other.signature);
            std::swap(component_offset_list, other.component_offset_list);
        }
        return *this;
    }

    [[nodiscard]] auto get_row_ptr(size_t row_index) const -> const void * {
        assert(row_index < len);
        return table_raw + row_index * row_size;
    }
    [[nodiscard]] auto get_row_ptr(size_t row_index) -> void * {
        assert(row_index < len);
        return table_raw + row_index * row_size;
    }

    [[nodiscard]] auto get_component_offset(size_t component_index) const -> size_t {
        assert(component_index < component_offset_list.len);
        return component_offset_list.data_raw[component_index];
    }

    [[nodiscard]] auto get_component_ptr(size_t row_index, size_t component_index) const -> const void * {
        return (const char *)get_row_ptr(row_index) + get_component_offset(component_index);
    }
    [[nodiscard]] auto get_component_ptr(size_t row_index, size_t component_index) -> void * {
        return (char *)get_row_ptr(row_index) + get_component_offset(component_index);
    }

    auto reserve(size_t new_cap) {
        if (new_cap <= cap) return;

        char *new_table_raw = (char *)std::malloc(new_cap * row_size);
        Entity *new_owner_list_raw = (Entity *)std::malloc(new_cap * sizeof(Entity));

        if (table_raw != nullptr) {
            std::memcpy(new_table_raw, table_raw, len * row_size);
            std::free(table_raw);
        }
        if (owner_list_raw != nullptr) {
            std::memcpy(new_owner_list_raw, owner_list_raw, len * sizeof(Entity));
            std::free(owner_list_raw);
        }

        table_raw = new_table_raw;
        owner_list_raw = new_owner_list_raw;
        cap = new_cap;
    }

    auto new_row(Entity entity_owner) -> size_t {
        if (len >= cap) {
            size_t new_cap = cap * 2;
            if (new_cap < 1) new_cap = 1;
            reserve(new_cap);
        }

        ++len;

        char *last_row_ptr = table_raw + (len - 1) * row_size;
        std::memset(last_row_ptr, 0, row_size);

        owner_list_raw[len - 1] = entity_owner;

        return len - 1;
    }

    // return the entity owner of the last row if it require to be moved by this
    auto remove_row(size_t index) -> std::optional<Entity> {
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

struct WorldImpl {
    struct EntityData {
        Signature signature;
        size_t table_row_index;
    };

    SlotMap<EntityData> entities_data;

    size_t component_count;
    FixedArr<ComponentData> component_data_list;

    std::unordered_map<Signature, size_t, SignatureHasher> signature_to_table_index_map;
    std::vector<WorldTable> tables;

    WorldImpl() = delete;
    ~WorldImpl() = default;

    WorldImpl(const WorldImpl &other) = delete;
    WorldImpl &operator=(const WorldImpl &other) = delete;

    WorldImpl(WorldImpl &&other) noexcept = default;
    WorldImpl &operator=(WorldImpl &&other) noexcept = default;

    template <typename... Ts> [[nodiscard]] static auto make() -> WorldImpl {
        WorldComponentsRegister<Ts...> component_register;

        size_t component_count = component_register.size();
        auto component_data_list = FixedArr<ComponentData>(component_count);

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (..., component_data_list.set(
                      Is, {component_register.template get_size<Is>(), component_register.template get_align<Is>()}));
        }(std::make_index_sequence<sizeof...(Ts)>{});

        return WorldImpl(component_count, std::move(component_data_list));
    }

    [[nodiscard]] auto new_entity() -> Entity { return entities_data.push(EntityData{Signature{}, 0}); }

    [[nodiscard]] auto has_entity(Entity entity) -> bool { return entities_data.has(entity); }

    [[nodiscard]] auto get_entity_signature(Entity entity) -> std::optional<Signature> {
        return entities_data.get(entity).transform([](auto data) { return data.signature; });
    }

    [[nodiscard]] auto is_entity_has_component(Entity entity, size_t component_index) -> bool {
        if (component_index >= component_count) return false; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return false;
        return entity_data_opt.value().signature.test(component_index);
    }

    // component_ptr is not stable pointer
    [[nodiscard]] auto get_component_ptr(Entity entity, size_t component_index) -> void * {
        if (component_index >= component_count) return nullptr; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr; // if entity not existed

        Signature signature = entity_data_opt.value().signature;
        assert((!signature.any() || signature_to_table_index_map.contains(signature)) &&
               "entity's signature should be empty or already existed");

        if (!signature.test(component_index)) return nullptr; // if entity's signature not has the component

        size_t table_index = signature_to_table_index_map.at(signature);
        assert(table_index < tables.size() && "table index shoule be existed");

        return tables[table_index].get_component_ptr(entity_data_opt->table_row_index, component_index);
    }
    [[nodiscard]] auto get_component_ptr(Entity entity, size_t component_index) const -> const void * {
        if (component_index >= component_count) return nullptr; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr; // if entity not existed

        Signature signature = entity_data_opt.value().signature;
        assert((!signature.any() || signature_to_table_index_map.contains(signature)) &&
               "entity's signature should be empty or already existed");

        if (!signature.test(component_index)) return nullptr; // if entity's signature not has the component

        size_t table_index = signature_to_table_index_map.at(signature);
        assert(table_index < tables.size() && "table index shoule be existed");

        return tables[table_index].get_component_ptr(entity_data_opt->table_row_index, component_index);
    }

    [[nodiscard]] auto has_component(Entity entity, size_t component_index) const -> bool {
        if (component_index >= component_count) return false; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return false;

        Signature signature = entity_data_opt.value().signature;
        assert((!signature.any() || signature_to_table_index_map.contains(signature)) &&
               "entity's signature should be empty or already existed");

        return signature.test(component_index);
    }

    auto add_component(Entity entity, size_t component_index) -> void * {
        if (component_index >= component_count) return nullptr; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return nullptr; // if entity not existed

        Signature cur_signature = entity_data_opt.value().signature;
        assert((!cur_signature.any() || signature_to_table_index_map.contains(cur_signature)) &&
               "entity's signature should be empty or already existed");

        // if current signature already has the component, return that component ptr
        if (cur_signature.test(component_index)) return get_component_ptr(entity, component_index);

        Signature new_signature = cur_signature;
        new_signature.set(component_index);

        // get new table, create new table if needed
        auto new_table_index_it = signature_to_table_index_map.find(new_signature);
        size_t new_table_index;
        if (new_table_index_it == signature_to_table_index_map.end()) {
            new_table_index = new_table(new_signature);
        } else {
            new_table_index = new_table_index_it->second;
        }

        // if current signature is empty or the entity has not existed in a table: just create new row in table
        if (!cur_signature.any()) {
            WorldTable &new_table = tables[new_table_index];
            new_table.new_row(entity);
            entities_data.set(entity, {new_signature, new_table.len - 1});
        }
        // else: move old data to new row, delete old row
        else {
            WorldTable &new_table = tables[new_table_index];
            new_table.new_row(entity);

            size_t cur_table_index = signature_to_table_index_map.at(cur_signature);
            WorldTable &cur_table = tables[cur_table_index];

            size_t cur_row_index = entity_data_opt.value().table_row_index;

            char *cur_row_ptr = (char *)cur_table.get_row_ptr(cur_row_index);
            char *new_row_ptr = (char *)new_table.get_row_ptr(new_table.len - 1);
            for (auto cur_signature_ull = cur_signature.to_ullong(); cur_signature_ull > 0;
                 cur_signature_ull &= (cur_signature_ull - 1)) {
                size_t t_component_index = __builtin_ctzll(cur_signature_ull);
                auto t_component_data = component_data_list.get(t_component_index);

                void *src = cur_row_ptr + cur_table.get_component_offset(t_component_index);
                void *dst = new_row_ptr + new_table.get_component_offset(t_component_index);
                std::memcpy(dst, src, t_component_data->size);
            }

            if (auto moved_entity_opt = cur_table.remove_row(cur_row_index)) {
                Entity moved_entity = moved_entity_opt.value();
                assert(entities_data.has(moved_entity) && "the entity in the last of table should already existed");

                auto moved_entity_data = entities_data.get(moved_entity).value();
                moved_entity_data.table_row_index = cur_row_index;
                entities_data.set(moved_entity, moved_entity_data);
            }

            entities_data.set(entity, {new_signature, new_table.len - 1});
        }

        WorldTable &new_table = tables[new_table_index];
        return new_table.get_component_ptr(new_table.len - 1, component_index);
    }

    auto add_component(Entity entity, std::initializer_list<size_t> component_index_list) -> void {
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return; // if entity not existed

        Signature cur_signature = entity_data_opt.value().signature;
        assert((!cur_signature.any() || signature_to_table_index_map.contains(cur_signature)) &&
               "entity's signature should be empty or already existed");

        Signature new_signature = cur_signature;
        for (size_t ci : component_index_list) {
            if (ci < component_count && !cur_signature.test(ci)) new_signature.set(ci);
        }
        if (new_signature == cur_signature) return;

        // get new table, create new table if needed
        auto new_table_index_it = signature_to_table_index_map.find(new_signature);
        size_t new_table_index;
        if (new_table_index_it == signature_to_table_index_map.end()) {
            new_table_index = new_table(new_signature);
        } else {
            new_table_index = new_table_index_it->second;
        }

        // if current signature is empty or the entity has not existed in a table: just create new row in table
        if (!cur_signature.any()) {
            WorldTable &new_table = tables[new_table_index];
            new_table.new_row(entity);
            entities_data.set(entity, {new_signature, new_table.len - 1});
        }
        // else: move old data to new row, delete old row
        else {
            WorldTable &new_table = tables[new_table_index];
            new_table.new_row(entity);

            size_t cur_table_index = signature_to_table_index_map.at(cur_signature);
            WorldTable &cur_table = tables[cur_table_index];

            size_t cur_row_index = entity_data_opt.value().table_row_index;

            char *cur_row_ptr = (char *)cur_table.get_row_ptr(cur_row_index);
            char *new_row_ptr = (char *)new_table.get_row_ptr(new_table.len - 1);
            for (auto cur_signature_ull = cur_signature.to_ullong(); cur_signature_ull > 0;
                 cur_signature_ull &= (cur_signature_ull - 1)) {
                size_t t_component_index = __builtin_ctzll(cur_signature_ull);
                auto t_component_data = component_data_list.get(t_component_index);

                void *src = cur_row_ptr + cur_table.get_component_offset(t_component_index);
                void *dst = new_row_ptr + new_table.get_component_offset(t_component_index);
                std::memcpy(dst, src, t_component_data->size);
            }

            if (auto moved_entity_opt = cur_table.remove_row(cur_row_index)) {
                Entity moved_entity = moved_entity_opt.value();
                assert(entities_data.has(moved_entity) && "the entity in the last of table should already existed");

                auto moved_entity_data = entities_data.get(moved_entity).value();
                moved_entity_data.table_row_index = cur_row_index;
                entities_data.set(moved_entity, moved_entity_data);
            }

            entities_data.set(entity, {new_signature, new_table.len - 1});
        }
    }

    auto remove_component(Entity entity, size_t component_index) -> bool {
        if (component_index >= component_count) return false; // component out of bound
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return false; // if entity not existed

        Signature cur_signature = entity_data_opt.value().signature;
        assert((!cur_signature.any() || signature_to_table_index_map.contains(cur_signature)) &&
               "entity's signature should be empty or already existed");

        if (!cur_signature.test(component_index)) return false;

        Signature new_signature = cur_signature;
        new_signature.reset(component_index);

        if (!new_signature.any()) {
            size_t cur_table_index = signature_to_table_index_map.at(cur_signature);
            WorldTable &cur_table = tables[cur_table_index];

            size_t cur_row_index = entity_data_opt.value().table_row_index;

            if (auto moved_entity_opt = cur_table.remove_row(cur_row_index)) {
                Entity moved_entity = moved_entity_opt.value();
                assert(entities_data.has(moved_entity) && "the entity in the last of table should already existed");

                auto moved_entity_data = entities_data.get(moved_entity).value();
                moved_entity_data.table_row_index = cur_row_index;
                entities_data.set(moved_entity, moved_entity_data);
            }

            entities_data.set(entity, {new_signature, 0});
            return true;
        }

        // get new table, create new table if needed
        auto new_table_index_it = signature_to_table_index_map.find(new_signature);
        size_t new_table_index;
        if (new_table_index_it == signature_to_table_index_map.end()) {
            new_table_index = new_table(new_signature);
        } else {
            new_table_index = new_table_index_it->second;
        }
        WorldTable &new_table = tables[new_table_index];
        new_table.new_row(entity);

        size_t cur_table_index = signature_to_table_index_map.at(cur_signature);
        WorldTable &cur_table = tables[cur_table_index];

        size_t cur_row_index = entity_data_opt.value().table_row_index;

        char *cur_row_ptr = (char *)cur_table.get_row_ptr(cur_row_index);
        char *new_row_ptr = (char *)new_table.get_row_ptr(new_table.len - 1);
        for (auto new_signature_ull = new_signature.to_ullong(); new_signature_ull > 0;
             new_signature_ull &= (new_signature_ull - 1)) {
            size_t t_component_index = __builtin_ctzll(new_signature_ull);
            auto t_component_data = component_data_list.get(t_component_index);

            void *src = cur_row_ptr + cur_table.get_component_offset(t_component_index);
            void *dst = new_row_ptr + new_table.get_component_offset(t_component_index);
            std::memcpy(dst, src, t_component_data->size);
        }

        if (auto moved_entity_opt = cur_table.remove_row(cur_row_index)) {
            Entity moved_entity = moved_entity_opt.value();
            assert(entities_data.has(moved_entity) && "the entity in the last of table should already existed");

            auto moved_entity_data = entities_data.get(moved_entity).value();
            moved_entity_data.table_row_index = cur_row_index;
            entities_data.set(moved_entity, moved_entity_data);
        }

        entities_data.set(entity, {new_signature, new_table.len - 1});

        return true;
    }

    auto remove_component(Entity entity, std::initializer_list<size_t> component_index_list) -> void {
        auto entity_data_opt = entities_data.get(entity);
        if (!entity_data_opt.has_value()) return; // if entity not existed

        Signature cur_signature = entity_data_opt.value().signature;
        assert((!cur_signature.any() || signature_to_table_index_map.contains(cur_signature)) &&
               "entity's signature should be empty or already existed");

        Signature new_signature = cur_signature;
        for (size_t ci : component_index_list) {
            if (ci < component_count && cur_signature.test(ci)) new_signature.reset(ci);
        }
        if (new_signature == cur_signature) return;

        if (!new_signature.any()) {
            size_t cur_table_index = signature_to_table_index_map.at(cur_signature);
            WorldTable &cur_table = tables[cur_table_index];

            size_t cur_row_index = entity_data_opt.value().table_row_index;

            if (auto moved_entity_opt = cur_table.remove_row(cur_row_index)) {
                Entity moved_entity = moved_entity_opt.value();
                assert(entities_data.has(moved_entity) && "the entity in the last of table should already existed");

                auto moved_entity_data = entities_data.get(moved_entity).value();
                moved_entity_data.table_row_index = cur_row_index;
                entities_data.set(moved_entity, moved_entity_data);
            }

            entities_data.set(entity, {new_signature, 0});
            return;
        }

        // get new table, create new table if needed
        auto new_table_index_it = signature_to_table_index_map.find(new_signature);
        size_t new_table_index;
        if (new_table_index_it == signature_to_table_index_map.end()) {
            new_table_index = new_table(new_signature);
        } else {
            new_table_index = new_table_index_it->second;
        }

        WorldTable &new_table = tables[new_table_index];
        new_table.new_row(entity);

        size_t cur_table_index = signature_to_table_index_map.at(cur_signature);
        WorldTable &cur_table = tables[cur_table_index];

        size_t cur_row_index = entity_data_opt.value().table_row_index;

        char *cur_row_ptr = (char *)cur_table.get_row_ptr(cur_row_index);
        char *new_row_ptr = (char *)new_table.get_row_ptr(new_table.len - 1);
        for (auto new_signature_ull = new_signature.to_ullong(); new_signature_ull > 0;
             new_signature_ull &= (new_signature_ull - 1)) {
            size_t t_component_index = __builtin_ctzll(new_signature_ull);
            auto t_component_data = component_data_list.get(t_component_index);

            void *src = cur_row_ptr + cur_table.get_component_offset(t_component_index);
            void *dst = new_row_ptr + new_table.get_component_offset(t_component_index);
            std::memcpy(dst, src, t_component_data->size);
        }

        if (auto moved_entity_opt = cur_table.remove_row(cur_row_index)) {
            Entity moved_entity = moved_entity_opt.value();
            assert(entities_data.has(moved_entity) && "the entity in the last of table should already existed");

            auto moved_entity_data = entities_data.get(moved_entity).value();
            moved_entity_data.table_row_index = cur_row_index;
            entities_data.set(moved_entity, moved_entity_data);
        }

        entities_data.set(entity, {new_signature, new_table.len - 1});
    }

private:
    explicit WorldImpl(size_t component_count, FixedArr<ComponentData> &&component_data_list)
        : entities_data(), component_count(component_count), component_data_list(std::move(component_data_list)),
          signature_to_table_index_map(), tables() {}

    auto new_table(Signature signature) -> size_t {
        assert(!signature_to_table_index_map.contains(signature) && "signature should not already existed");

        tables.push_back(WorldTable(signature, component_count, component_data_list));
        signature_to_table_index_map.insert({signature, tables.size() - 1});

        return tables.size() - 1;
    }
};

export template <typename... Ts> struct World;

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
            WorldTable *table_ref;
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

export template <typename... Ts> struct World {
    WorldImpl world_impl;
    using component_register_t = WorldComponentsRegister<Ts...>;

    explicit World() : world_impl(WorldImpl::make<Ts...>()) {}
    ~World() = default;

    World(const World &other) = delete;
    World &operator=(const World &other) = delete;

    World(World &&other) noexcept = default;
    World &operator=(World &&other) noexcept = default;

    [[nodiscard]] auto new_entity() -> Entity { return world_impl.new_entity(); }

    [[nodiscard]] auto has_entity(Entity entity) -> bool { return world_impl.has_entity(entity); }

    [[nodiscard]] auto get_entity_signature(Entity entity) -> std::optional<Signature> {
        return world_impl.get_entity_signature(entity);
    }

    template <typename T>
        requires(component_register_t::template has<T>())
    [[nodiscard]] auto is_entity_has_component(Entity entity) -> bool {
        return world_impl.is_entity_has_component(entity, component_register_t::template get_index<T>());
    }

    template <typename T>
        requires(component_register_t::template has<T>())
    [[nodiscard]] auto get_component(Entity entity) const -> std::optional<T> {
        const void *ptr = world_impl.get_component_ptr(entity, component_register_t::template get_index<T>());
        if (ptr == nullptr) return {};
        return *(T *)ptr;
    }

    template <typename T>
        requires(component_register_t::template has<T>())
    [[nodiscard]] auto get_component_ptr(Entity entity) -> T * {
        return (T *)world_impl.get_component_ptr(entity, component_register_t::template get_index<T>());
    }
    template <typename T>
        requires(component_register_t::template has<T>())
    [[nodiscard]] auto get_component_ptr(Entity entity) const -> const T * {
        return (const T *)world_impl.get_component_ptr(entity, component_register_t::template get_index<T>());
    }

    template <typename T>
        requires(component_register_t::template has<T>())
    [[nodiscard]] auto has_component(Entity entity) const -> bool {
        return world_impl.has_component(entity, component_register_t::template get_index<T>());
    }

    template <typename T>
        requires(component_register_t::template has<T>())
    auto add_component(Entity entity) -> T * {
        return (T *)world_impl.add_component(entity, component_register_t::template get_index<T>());
    }
    template <typename... Us>
        requires(sizeof...(Us) > 1 && (component_register_t::template has<Us>() && ...))
    auto add_component(Entity entity) -> void {
        world_impl.add_component(entity, {component_register_t::template get_index<Us>()...});
    }

    template <typename T>
        requires(component_register_t::template has<T>())
    auto remove_component(Entity entity) -> bool {
        return world_impl.remove_component(entity, component_register_t::template get_index<T>());
    }
    template <typename... Us>
        requires(sizeof...(Us) > 1 && (component_register_t::template has<Us>() && ...))
    auto remove_component(Entity entity) -> void {
        world_impl.remove_component(entity, {component_register_t::template get_index<Us>()...});
    }

    [[nodiscard]] auto is_entity_has_component(Entity entity, size_t component_index) -> bool {
        return world_impl.is_entity_has_component(entity, component_index);
    }

    [[nodiscard]] auto get_component_ptr(Entity entity, size_t component_index) -> void * {
        return world_impl.get_component_ptr(entity, component_index);
    }
    [[nodiscard]] auto get_component_ptr(Entity entity, size_t component_index) const -> const void * {
        return world_impl.get_component_ptr(entity, component_index);
    }

    [[nodiscard]] auto has_component(Entity entity, size_t component_index) const -> bool {
        return world_impl.has_component(entity, component_index);
    }

    auto add_component(Entity entity, size_t component_index) -> void * {
        return world_impl.add_component(entity, component_index);
    }
    auto add_component(Entity entity, std::initializer_list<size_t> component_index_list) -> void {
        world_impl.add_component(entity, component_index_list);
    }

    auto remove_component(Entity entity, size_t component_index) -> bool {
        return world_impl.remove_component(entity, component_index);
    }
    auto remove_component(Entity entity, std::initializer_list<size_t> component_index_list) -> void {
        world_impl.remove_component(entity, component_index_list);
    }

    [[nodiscard]] auto query_builder() -> WorldQueryBuilder<Ts...> { return WorldQueryBuilder<Ts...>(this); }
};

} // namespace cactus
