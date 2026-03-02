module;

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <optional>

export module Ecs:World;

import :Common;
import :Component;
import :Signature;
import :Archetype;

import SlotMap;
import FreelistVector;

namespace cactus::ecs {

constexpr auto align_up(uintptr_t ptr, size_t alignment) -> uintptr_t {
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    return (ptr + alignment - 1) & ~(alignment - 1);
}
constexpr auto align_up_offset(void *root_ptr, size_t offset, size_t alignment) -> size_t {
    uintptr_t ptr = (uintptr_t)root_ptr + offset;
    return align_up(ptr, alignment) - (uintptr_t)root_ptr;
}

export struct World {
    struct EntityStatus {
        SignatureID signature_id;
        size_t row;
    };

    FreelistVector<EntityStatus> entities_status;
    bstu::unordered_flat_map<SignatureID, ArchetypeTable> archetype_tables;
    ComponentAtlas component_atlas;
    SignatureAtlas signature_atlas;

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
    auto emplace_component(Entity entity, Args... args) -> std::expected<void, bool> {
        if (!contains_entity(entity)) return std::unexpected{0};
        component_atlas.register_component<T>();
        T data{std::forward<Args>(args)...};
        add_or_set_component(entity, component_id<T>(), &data);
    }

    template <typename T>
    auto erase_component(Entity entity) -> std::expected<bool, bool> {
        if (!contains_entity(entity)) return std::unexpected{0};
        return try_remove_component(entity, component_id<T>());
    }

    template <typename T>
    auto add_or_set_component(Entity entity, ComponentID component_id, T *data) {
        assert(entities_status.contains(entity));
        auto signature_id = entities_status[entity].signature_id;
        assert(archetype_tables.contains(signature_id) || signature_id == EMPTY_SIGNATURE_ID);

        if (signature_atlas.contains_component(signature_id, component_id)) {
            auto &table = archetype_tables.at(signature_id);
            const auto column_offset = row_offset_of_component(signature_id, component_id);

            memcpy((std::byte *)table.get_row_ptr(entities_status[entity].row) + column_offset,
                   data, sizeof(T));

            return;
        }

        auto new_signature_id =
            signature_atlas.create_or_get_signature_by_add_component(signature_id, component_id);

        if (signature_id == EMPTY_SIGNATURE_ID) {
            ArchetypeTable &new_archetype_table =
                archetype_tables.try_emplace(new_signature_id, signature_row_size(new_signature_id))
                    .first->second;

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
            archetype_tables.try_emplace(new_signature_id, signature_row_size(new_signature_id))
                .first->second;

        auto old_row = entities_status[entity].row;
        auto new_row = new_archetype_table.create_row();

        auto *old_row_ptr = old_archetype_table.get_row_ptr(old_row);
        auto *new_row_ptr = new_archetype_table.get_row_ptr(new_row);

        const auto &old_signature_container = *signature_atlas.get_container(signature_id);
        const auto &new_signature_container = *signature_atlas.get_container(new_signature_id);
        size_t old_cur_row_offset = 0, new_cur_row_offset = 0;
        for (size_t i = 0; i < old_signature_container.size; ++i) {
            auto cur_component_id = old_signature_container.ptr[i];
            auto cur_componet_size = component_atlas.get_component_size(cur_component_id);
            auto cur_componet_align = component_atlas.get_component_alignment(cur_component_id);

            if (new_signature_container.ptr[i] == component_id) {
                memcpy(new_row_ptr, old_row_ptr, old_cur_row_offset);

                new_cur_row_offset = align_up_offset(new_row_ptr, new_cur_row_offset, alignof(T));

                memcpy((std::byte *)new_row_ptr + new_cur_row_offset, data, sizeof(T));

                new_cur_row_offset += sizeof(T);

                old_cur_row_offset =
                    align_up_offset(old_row_ptr, old_cur_row_offset, cur_componet_align);
                new_cur_row_offset =
                    align_up_offset(new_row_ptr, new_cur_row_offset, cur_componet_align);

                memcpy((std::byte *)new_row_ptr + new_cur_row_offset,
                       (std::byte *)old_row_ptr + old_cur_row_offset,
                       old_archetype_table.row_size - old_cur_row_offset);

                break;
            }

            old_cur_row_offset =
                align_up_offset(old_row_ptr, old_cur_row_offset, cur_componet_align)
                + cur_componet_size;
            new_cur_row_offset =
                align_up_offset(new_row_ptr, new_cur_row_offset, cur_componet_align)
                + cur_componet_size;
        }
        if (new_cur_row_offset == 0) {
            memcpy(new_row_ptr, old_row_ptr, old_archetype_table.row_size);
            memcpy((std::byte *)new_row_ptr + old_archetype_table.row_size, data, sizeof(T));
        }

        new_archetype_table.entity_owned_list_ptr[new_row] = entity;

        entities_status[entity].signature_id = new_signature_id;
        entities_status[entity].row = new_row;

        auto old_last_row = old_archetype_table.row_count - 1;
        if (old_last_row != old_row) {
            auto *old_last_row_ptr = old_archetype_table.get_row_ptr(old_last_row);

            memcpy(old_row_ptr, old_last_row_ptr, old_archetype_table.row_size);
            old_archetype_table.entity_owned_list_ptr[old_row] =
                old_archetype_table.entity_owned_list_ptr[old_last_row];

            entities_status[old_archetype_table.entity_owned_list_ptr[old_last_row]].row = old_row;
        }
        --old_archetype_table.row_count;
    }

    auto try_remove_component(Entity entity, ComponentID component_id) -> bool {
        assert(entities_status.contains(entity));
        auto signature_id = entities_status[entity].signature_id;
        assert(archetype_tables.contains(signature_id) || signature_id == EMPTY_SIGNATURE_ID);

        auto component_size = component_atlas.get_component_size(component_id);
        auto component_align = component_atlas.get_component_alignment(component_id);

        if (!signature_atlas.contains_component(signature_id, component_id)) return false;

        auto new_signature_id =
            signature_atlas.create_or_get_signature_by_remove_component(signature_id, component_id);

        if (new_signature_id == EMPTY_SIGNATURE_ID) {
            ArchetypeTable &old_archetype_table = archetype_tables.at(new_signature_id);

            auto old_row = entities_status[entity].row;

            auto old_last_row = old_archetype_table.row_count - 1;
            if (old_last_row != old_row) {
                auto *old_row_ptr = old_archetype_table.get_row_ptr(old_row);
                auto *old_last_row_ptr = old_archetype_table.get_row_ptr(old_last_row);

                memcpy(old_row_ptr, old_last_row_ptr, old_archetype_table.row_size);
                old_archetype_table.entity_owned_list_ptr[old_row] =
                    old_archetype_table.entity_owned_list_ptr[old_last_row];

                entities_status[old_archetype_table.entity_owned_list_ptr[old_last_row]].row =
                    old_row;
            }
            --old_archetype_table.row_count;

            entities_status[entity].signature_id = EMPTY_SIGNATURE_ID;
            entities_status[entity].row = 0;

            return true;
        }

        ArchetypeTable &old_archetype_table = archetype_tables.at(signature_id);
        ArchetypeTable &new_archetype_table =
            archetype_tables.try_emplace(new_signature_id, signature_row_size(new_signature_id))
                .first->second;

        auto old_row = entities_status[entity].row;
        auto new_row = new_archetype_table.create_row();

        auto *old_row_ptr = old_archetype_table.get_row_ptr(old_row);
        auto *new_row_ptr = new_archetype_table.get_row_ptr(new_row);

        const auto &old_signature_container = *signature_atlas.get_container(signature_id);
        const auto &new_signature_container = *signature_atlas.get_container(new_signature_id);
        size_t old_cur_row_offset = 0, new_cur_row_offset = 0;
        for (size_t i = 0; i < old_signature_container.size; ++i) {
            auto cur_component_id = old_signature_container.ptr[i];
            auto cur_componet_size = component_atlas.get_component_size(cur_component_id);
            auto cur_componet_align = component_atlas.get_component_alignment(cur_component_id);

            if (new_signature_container.ptr[i] == component_id) {
                memcpy(new_row_ptr, old_row_ptr, old_cur_row_offset);

                new_cur_row_offset =
                    align_up_offset(new_row_ptr, new_cur_row_offset, component_align)
                    + component_size;

                old_cur_row_offset =
                    align_up_offset(old_row_ptr, old_cur_row_offset, cur_componet_align);
                new_cur_row_offset =
                    align_up_offset(new_row_ptr, new_cur_row_offset, cur_componet_align);

                memcpy((std::byte *)new_row_ptr + new_cur_row_offset,
                       (std::byte *)old_row_ptr + old_cur_row_offset,
                       old_archetype_table.row_size - old_cur_row_offset);

                break;
            }

            old_cur_row_offset =
                align_up_offset(old_row_ptr, old_cur_row_offset, cur_componet_align)
                + cur_componet_size;
            new_cur_row_offset =
                align_up_offset(new_row_ptr, new_cur_row_offset, cur_componet_align)
                + cur_componet_size;
        }

        const auto new_column_offset = row_offset_of_component(new_signature_id, component_id);

        return true;
    }

    [[nodiscard]] auto signature_row_size(SignatureID sid) const -> size_t {
        assert(sid < signature_atlas.signature_containers.size());

        size_t size = 0;
        const auto &signature_container = *signature_atlas.get_container(sid);
        for (size_t i = 0; i < signature_container.size; ++i) {
            auto csize = component_atlas.get_component_size(signature_container.ptr[i]);
            auto calign = component_atlas.get_component_alignment(signature_container.ptr[i]);
            size = align_up(size, calign) + csize;
        }
        return size;
    }

    [[nodiscard]] auto row_offset_of_component(SignatureID sid, ComponentID cid) const -> size_t {
        assert(sid < signature_atlas.signature_containers.size());
        assert(signature_atlas.contains_component(sid, cid));

        size_t offset = 0;
        auto *cur_component_ptr = signature_atlas.signature_containers.at(sid).ptr;
        while (*cur_component_ptr != cid) {
            assert(cur_component_ptr - signature_atlas.signature_containers.at(sid).ptr
                   < signature_atlas.signature_containers.at(sid).size - 1);

            auto csize = component_atlas.get_component_size(*cur_component_ptr);
            auto calign = component_atlas.get_component_alignment(*cur_component_ptr);
            offset = align_up(offset, calign) + csize;

            ++cur_component_ptr;
        }
        return align_up(offset, component_atlas.get_component_alignment(*cur_component_ptr));
    }
};

} // namespace cactus::ecs
