module;

#include <cassert>
#include <cstddef>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <tuplet/tuple.hpp>

export module Ecs:World;

import :Common;
import :Component;
import :Signature;
import :Archetype;

import SlotMap;

namespace cactus::ecs {

export struct World {
    struct EntityStatus {
        SignatureID signature_id = (SignatureID)-1;
        size_t row;
    };

    SlotMap<EntityStatus> entities_status;
    bstu::unordered_flat_map<SignatureID, ArchetypeTable> archetype_tables;
    ComponentAtlas component_atlas;
    SignatureAtlas signature_atlas;

    World() = default;
    World(const World &) = delete;
    World &operator=(const World &) = delete;
    World(World &&other) noexcept = delete; // TODO world is moveable
    World &operator=(World &&other) noexcept = delete;
    ~World() {
        for (auto &&[_, table] : archetype_tables) {
            table.free();
        }
        signature_atlas.free();
    }

    // FIXME: DEBUG STUFF
    std::string debug_str;
    [[nodiscard]] auto get_debug() -> const std::string & {
        debug_str = "";

        debug_str += "Entites:\n";
        int __i = 0;
        for (auto &x : entities_status) {
            debug_str +=
                std::format("  {}: signature_id {}; row {}\n", __i++, x.signature_id, x.row);
        }

        debug_str += "Components:\n";
        for (auto &[id, p] : component_atlas.component_property_map) {
            debug_str += std::format("  {}: size {} - alignment {}", id, tuplet::get<0>(p),
                                     tuplet::get<1>(p));
            debug_str += "\n";
        }

        debug_str += "Signatures:\n";
        __i = 0;
        for (auto &x : signature_atlas.signature_datas) {
            debug_str += std::format("  signature {} constains: ", __i);
            for (size_t i = 0; i < x.size; ++i) debug_str += std::format("{} ", x.ptr[i]);
            debug_str += "\n";
            ++__i;
        }

        debug_str += "Archetype tables:\n";
        for (auto &&[id, table] : archetype_tables) {
            debug_str += std::format("  signature_id {}: table with {} rows; each size {}\n", id,
                                     table.row_count, table.row_size);

            for (size_t i = 0; i < table.row_count; ++i) {
                auto *row_ptr = table.get_row_ptr(i);
                debug_str += std::format("    row {} owned by {} at {}: ", i,
                                         tuplet::get<0>(table.entity_owned_list_ptr[i]), row_ptr);
                for (std::size_t j = 0; j < table.row_size; ++j) {
                    debug_str += std::format("{:02x} ", *((char *)row_ptr + j));
                }
                debug_str += "\n";
            }
        }

        return debug_str;
    }

    [[nodiscard]] auto create_entity() -> Entity {
        auto entity = entities_status.emplace(EMPTY_SIGNATURE_ID, 0);
        return entity;
    }

    [[nodiscard]] auto contains_entity(Entity entity) -> bool {
        return entities_status.contains(entity);
    }

    template <typename T>
    [[nodiscard]] auto get_component(Entity entity) -> std::optional<T *> {
        if (!contains_entity(entity)) return {};

        auto id = component_id<T>();
        auto signature_id = entities_status[entity].signature_id;

        auto table_iter = archetype_tables.find(signature_id);
        if (table_iter == archetype_tables.end()) return {};

        return (T *)((std::byte *)table_iter->second.get_row_ptr(entities_status[entity].row)
                     + row_offset_of_component(signature_id, id));
    }

    template <typename T>
    [[nodiscard]] auto contains_component(Entity entity) -> bool {
        if (!contains_entity(entity)) return {};

        auto id = component_id<T>();
        auto signature_id = entities_status[entity].signature_id;

        return signature_atlas.contains_component(signature_id, id);
    }

    template <typename T, typename... Args>
    auto emplace_component(Entity entity, Args... args) -> bool {
        if (!contains_entity(entity)) return false;
        component_atlas.register_component<T>();
        T data{std::forward<Args>(args)...};
        add_or_set_component_raw(entity, component_id<T>(), &data);
        return true;
    }

    template <typename T>
    auto remove_component(Entity entity) -> bool {
        if (!contains_entity(entity)) return false;
        return try_remove_component_raw(entity, component_id<T>());
    }

    template <typename T>
    auto add_or_set_component_raw(Entity entity, ComponentID component_id, T *data) {
        assert(entities_status.contains(entity));
        auto signature_id = entities_status[entity].signature_id;
        assert(archetype_tables.contains(signature_id) || signature_id == EMPTY_SIGNATURE_ID);

        // If component existed; just assign
        if (signature_atlas.contains_component(signature_id, component_id)) {
            auto &table = archetype_tables.at(signature_id);
            const auto offset = row_offset_of_component(signature_id, component_id);

            memcpy((std::byte *)table.get_row_ptr(entities_status[entity].row) + offset, data,
                   sizeof(T));

            return;
        }

        auto new_signature_id =
            signature_atlas.create_or_get_signature_by_add_component(signature_id, component_id);

        // If previously, entity has no component; create new row, no need copying data
        if (signature_id == EMPTY_SIGNATURE_ID) {
            ArchetypeTable &new_archetype_table =
                archetype_tables.try_emplace(new_signature_id).first->second;
            new_archetype_table.row_size = signature_row_size(new_signature_id);
            new_archetype_table.row_align = signature_row_align(new_signature_id);

            auto new_row = new_archetype_table.create_row();
            auto *new_row_ptr = new_archetype_table.get_row_ptr(new_row);

            memcpy(new_row_ptr, data, sizeof(T));

            new_archetype_table.entity_owned_list_ptr[new_row] = entity;

            entities_status[entity].signature_id = new_signature_id;
            entities_status[entity].row = new_row;

            return;
        }

        ArchetypeTable &old_archetype_table = archetype_tables.at(signature_id);
        ArchetypeTable &new_archetype_table =
            archetype_tables.try_emplace(new_signature_id).first->second;
        new_archetype_table.row_size = signature_row_size(new_signature_id);
        new_archetype_table.row_align = signature_row_align(new_signature_id);

        auto old_row = entities_status[entity].row;
        auto new_row = new_archetype_table.create_row();

        auto *old_row_ptr = old_archetype_table.get_row_ptr(old_row);
        auto *new_row_ptr = new_archetype_table.get_row_ptr(new_row);

        const auto &old_signature_data = *signature_atlas.get_data(signature_id);
        const auto &new_signature_data = *signature_atlas.get_data(new_signature_id);
        size_t old_cur_row_offset = 0, new_cur_row_offset = 0;
        bool flag = false;
        for (size_t i = 0; i < old_signature_data.size; ++i) {
            auto cur_component_id = old_signature_data.ptr[i];
            auto cur_componet_size = component_atlas.get_component_size(cur_component_id);
            auto cur_componet_align = component_atlas.get_component_alignment(cur_component_id);

            if (new_signature_data.ptr[i] == component_id) {
                new_cur_row_offset = align_up_offset(new_cur_row_offset, alignof(T));

                memcpy((std::byte *)new_row_ptr + new_cur_row_offset, data, sizeof(T));

                new_cur_row_offset += sizeof(T);

                flag = true;
            }
            old_cur_row_offset = align_up_offset(old_cur_row_offset, cur_componet_align);
            new_cur_row_offset = align_up_offset(new_cur_row_offset, cur_componet_align);

            memcpy((std::byte *)new_row_ptr + new_cur_row_offset,
                   (std::byte *)old_row_ptr + old_cur_row_offset, cur_componet_size);

            old_cur_row_offset += cur_componet_size;
            new_cur_row_offset += cur_componet_size;
        }
        if (!flag) { // Component data itself has not been copied, append it to the last of the row
            new_cur_row_offset = align_up_offset(new_cur_row_offset, alignof(T));
            memcpy((std::byte *)new_row_ptr + new_cur_row_offset, data, sizeof(T));
        }

        // Entity data
        new_archetype_table.entity_owned_list_ptr[new_row] = entity;

        entities_status[entity].signature_id = new_signature_id;
        entities_status[entity].row = new_row;

        // Remove row, clean up old table
        auto old_last_row = old_archetype_table.row_count - 1;
        if (old_last_row != old_row)
            entities_status[old_archetype_table.entity_owned_list_ptr[old_last_row]].row = old_row;

        old_archetype_table.remove_row(old_row);
    }

    auto try_remove_component_raw(Entity entity, ComponentID component_id) -> bool {
        assert(entities_status.contains(entity));
        auto signature_id = entities_status[entity].signature_id;
        assert(archetype_tables.contains(signature_id) || signature_id == EMPTY_SIGNATURE_ID);

        auto component_size = component_atlas.get_component_size(component_id);
        auto component_align = component_atlas.get_component_alignment(component_id);

        // If entity not contains component
        if (!signature_atlas.contains_component(signature_id, component_id)) return false;

        auto new_signature_id =
            signature_atlas.create_or_get_signature_by_remove_component(signature_id, component_id);

        // If after remove, entity contains no component; just remove the row completely
        if (new_signature_id == EMPTY_SIGNATURE_ID) {
            ArchetypeTable &old_archetype_table = archetype_tables.at(signature_id);

            auto old_row = entities_status[entity].row;

            auto old_last_row = old_archetype_table.row_count - 1;
            if (old_last_row != old_row) {
                entities_status[old_archetype_table.entity_owned_list_ptr[old_last_row]].row =
                    old_row;
            }
            old_archetype_table.remove_row(old_row);

            entities_status[entity].signature_id = EMPTY_SIGNATURE_ID;
            entities_status[entity].row = 0;

            return true;
        }

        ArchetypeTable &old_archetype_table = archetype_tables.at(signature_id);
        ArchetypeTable &new_archetype_table =
            archetype_tables.try_emplace(new_signature_id).first->second;
        new_archetype_table.row_size = signature_row_size(new_signature_id);
        new_archetype_table.row_align = signature_row_align(new_signature_id);

        auto old_row = entities_status[entity].row;
        auto new_row = new_archetype_table.create_row();

        auto *old_row_ptr = old_archetype_table.get_row_ptr(old_row);
        auto *new_row_ptr = new_archetype_table.get_row_ptr(new_row);

        const auto &old_signature_data = *signature_atlas.get_data(signature_id);
        const auto &new_signature_data = *signature_atlas.get_data(new_signature_id);
        size_t old_cur_row_offset = 0, new_cur_row_offset = 0;
        for (size_t i = 0; i < old_signature_data.size; ++i) {
            auto cur_component_id = old_signature_data.ptr[i];
            auto cur_componet_size = component_atlas.get_component_size(cur_component_id);
            auto cur_componet_align = component_atlas.get_component_alignment(cur_component_id);

            if (old_signature_data.ptr[i] == component_id) {
                new_cur_row_offset =
                    align_up_offset(new_cur_row_offset, component_align) + component_size;
            }

            old_cur_row_offset = align_up_offset(old_cur_row_offset, cur_componet_align);
            new_cur_row_offset = align_up_offset(new_cur_row_offset, cur_componet_align);

            memcpy((std::byte *)new_row_ptr + new_cur_row_offset,
                   (std::byte *)old_row_ptr + old_cur_row_offset, cur_componet_size);

            old_cur_row_offset += cur_componet_size;
            new_cur_row_offset += cur_componet_size;
        }

        // Entity data
        new_archetype_table.entity_owned_list_ptr[new_row] = entity;

        entities_status[entity].signature_id = new_signature_id;
        entities_status[entity].row = new_row;

        // Remove row, clean up old table
        auto old_last_row = old_archetype_table.row_count - 1;
        if (old_last_row != old_row)
            entities_status[old_archetype_table.entity_owned_list_ptr[old_last_row]].row = old_row;

        old_archetype_table.remove_row(old_row);

        return true;
    }

    [[nodiscard]] auto signature_row_size(SignatureID sid) const -> size_t {
        assert(sid < signature_atlas.signature_datas.size());

        size_t size = 0;
        const auto &signature_data = *signature_atlas.get_data(sid);
        for (size_t i = 0; i < signature_data.size; ++i) {
            auto csize = component_atlas.get_component_size(signature_data.ptr[i]);
            auto calign = component_atlas.get_component_alignment(signature_data.ptr[i]);
            size = align_up_offset(size, calign) + csize;
        }
        return size;
    }

    [[nodiscard]] auto signature_row_align(SignatureID sid) const -> size_t {
        assert(sid < signature_atlas.signature_datas.size());

        size_t align = 0;
        const auto &signature_data = *signature_atlas.get_data(sid);
        for (size_t i = 0; i < signature_data.size; ++i) {
            auto calign = component_atlas.get_component_alignment(signature_data.ptr[i]);
            align = std::max(align, calign);
        }
        return align;
    }

    [[nodiscard]] auto row_offset_of_component(SignatureID sid, ComponentID cid) const -> size_t {
        assert(sid < signature_atlas.signature_datas.size());
        assert(signature_atlas.contains_component(sid, cid));

        size_t offset = 0;
        auto *cur_component_ptr = signature_atlas.signature_datas.at(sid).ptr;
        while (*cur_component_ptr != cid) {
            assert(cur_component_ptr - signature_atlas.signature_datas.at(sid).ptr
                   < signature_atlas.signature_datas.at(sid).size - 1);

            auto csize = component_atlas.get_component_size(*cur_component_ptr);
            auto calign = component_atlas.get_component_alignment(*cur_component_ptr);
            offset = align_up_offset(offset, calign) + csize;

            ++cur_component_ptr;
        }
        return align_up_offset(offset, component_atlas.get_component_alignment(*cur_component_ptr));
    }
};

} // namespace cactus::ecs
