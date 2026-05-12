module;

#include <cassert>

export module cactus.core.ecs:archetype;

import :component;
import :signature;
import :entity;

import std;

using size_t = std::size_t;

namespace cactus {

export [[nodiscard]] auto align_up(size_t p, size_t align) -> size_t;
[[nodiscard]] auto handle_cal_row_size(ComponentRegistry *component_registry_ref, const Signature &signature) -> size_t;

export struct Archetype {
    ComponentRegistry *component_registry_ref;
    SignatureAtlas *signature_atlas_ref;
    SignatureAtlasKey signature_key;

    char *table_raw;
    Entity *owner_raw;
    size_t row_size;
    size_t len;
    size_t cap;

    std::flat_map<ComponentKey, size_t> component_offset_list;

    [[nodiscard]] static auto make(ComponentRegistry *component_registry_ref, SignatureAtlas *signature_atlas_ref,
                                   SignatureAtlasKey signature_key) -> Archetype {
        Archetype archetype =
            Archetype{.component_registry_ref = component_registry_ref,
                      .signature_atlas_ref = signature_atlas_ref,
                      .signature_key = signature_key,
                      .table_raw = nullptr,
                      .owner_raw = nullptr,
                      .row_size = handle_cal_row_size(component_registry_ref, signature_atlas_ref->get(signature_key)),
                      .len = 0,
                      .cap = 0};
        size_t offset = 0;
        for (ComponentKey c : SignatureView(signature_atlas_ref->get(signature_key))) {
            ComponentData c_data = component_registry_ref->get_component_data(c);

            offset = align_up(offset, c_data.align);

            archetype.component_offset_list[c] = offset;

            offset += c_data.size;
        }

        return archetype;
    }

    auto destroy() const {
        std::free(table_raw);
        std::free(owner_raw);
    }

    auto reserve(size_t new_cap) {
        if (new_cap <= cap) return;

        char *new_table_raw = (char *)std::malloc(new_cap * row_size);
        Entity *new_owner_raw = (Entity *)std::malloc(new_cap * sizeof(Entity));

        if (table_raw != nullptr) {
            std::memcpy(new_table_raw, table_raw, len * row_size);
            std::free(table_raw);
        }
        if (owner_raw != nullptr) {
            std::memcpy(new_owner_raw, owner_raw, len * sizeof(Entity));
            std::free(owner_raw);
        }

        table_raw = new_table_raw;
        owner_raw = new_owner_raw;
        cap = new_cap;
    }
    auto append(const Entity &entity_owner) -> size_t {
        if (len >= cap) {
            size_t new_cap = cap * 2;
            if (new_cap < 4) new_cap = 4;
            reserve(new_cap);
        }

        ++len;

        char *last_row_ptr = table_raw + (len - 1) * row_size;
        std::memset(last_row_ptr, 0, row_size);

        owner_raw[len - 1] = entity_owner;

        return len - 1;
    }
    auto remove(size_t index) -> bool {
        if (index >= len) return false;

        char *t_row_ptr = table_raw + index * row_size;
        char *last_row_ptr = table_raw + (len - 1) * row_size;
        std::memcpy(t_row_ptr, last_row_ptr, row_size);

        owner_raw[index] = owner_raw[len - 1];

        --len;

        return true;
    }

    [[nodiscard]] auto get_row_ptr(size_t row_index) const -> const void * {
        assert(row_index < len);
        return table_raw + row_index * row_size;
    }
    [[nodiscard]] auto get_row_ptr(size_t row_index) -> void * {
        assert(row_index < len);
        return table_raw + row_index * row_size;
    }
    [[nodiscard]] auto get_component_row_offset(ComponentKey component) const -> size_t {
        assert(component_offset_list.contains(component));
        return component_offset_list.at(component);
    }
    [[nodiscard]] auto get_component_ptr(size_t row_index, ComponentKey component) const -> const void * {
        return (const char *)get_row_ptr(row_index) + get_component_row_offset(component);
    }
    [[nodiscard]] auto get_component_ptr(size_t row_index, ComponentKey component) -> void * {
        return (char *)get_row_ptr(row_index) + get_component_row_offset(component);
    }

    [[nodiscard]] auto get_owner(size_t row_index) const -> Entity {
        assert(row_index < len);
        return owner_raw[row_index];
    }
};

[[nodiscard]] auto align_up(size_t p, size_t align) -> size_t {
    assert((align & (align - 1)) == 0);
    return (p + align - 1) & ~(align - 1);
}

[[nodiscard]] auto handle_cal_row_size(ComponentRegistry *component_registry_ref, const Signature &signature) -> size_t {
    size_t res = 0;

    for (ComponentKey c : SignatureView(signature)) {
        assert(component_registry_ref->has_key(c));

        ComponentData c_data = component_registry_ref->get_component_data(c);

        res = align_up(res, c_data.align) + c_data.size;
    }

    if (signature.any()) {
        ComponentKey c = *SignatureView(signature).begin();

        ComponentData c_data = component_registry_ref->get_component_data(c);

        res = align_up(res, c_data.align);
    }

    return res;
}

} // namespace cactus
