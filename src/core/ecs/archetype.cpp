module;

#include <cassert>

export module cactus.core.ecs:archetype;

import :component;
import :signature;

import std;

using size_t = std::size_t;

namespace cactus {

export [[nodiscard]] auto align_up(size_t p, size_t align) -> size_t;
[[nodiscard]] auto handle_cal_row_size(ComponentAtlas *component_atlas_ref, const Signature &signature) -> size_t;

export struct Archetype {
    ComponentAtlas *component_atlas_ref;
    SignatureAtlas *signature_atlas_ref;
    SignatureAtlasKey signature_key;

    char *table_raw;
    size_t row_size;
    size_t len;
    size_t cap;

    [[nodiscard]] static auto make(ComponentAtlas *component_atlas_ref, SignatureAtlas *signature_atlas_ref,
                                   SignatureAtlasKey signature_key) -> Archetype {
        return Archetype{.component_atlas_ref = component_atlas_ref,
                         .signature_atlas_ref = signature_atlas_ref,
                         .signature_key = signature_key,
                         .table_raw = nullptr,
                         .row_size = handle_cal_row_size(component_atlas_ref, signature_atlas_ref->get(signature_key)),
                         .len = 0,
                         .cap = 0};
    }

    auto destroy() const { std::free(table_raw); }

    auto reserve(size_t new_cap) {
        if (new_cap <= cap) return;

        char *new_table_raw = (char *)std::malloc(new_cap * row_size);

        if (table_raw != nullptr) {
            std::memcpy(new_table_raw, table_raw, len * row_size);
            std::free(table_raw);
        }

        table_raw = new_table_raw;
        cap = new_cap;
    }
    auto append() -> size_t {
        if (len >= cap) {
            size_t new_cap = cap * 2;
            if (new_cap < 4) new_cap = 4;
            reserve(new_cap);
        }

        ++len;

        char *last_row_ptr = table_raw + (len - 1) * row_size;
        std::memset(last_row_ptr, 0, row_size);

        return len - 1;
    }
    auto remove(size_t index) -> bool {
        if (index >= len) return false;

        char *t_row_ptr = table_raw + index * row_size;
        char *last_row_ptr = table_raw + (len - 1) * row_size;
        std::memcpy(t_row_ptr, last_row_ptr, row_size);

        --len;

        return true;
    }

    [[nodiscard]] auto get_row_ptr(size_t index) const -> const void * {
        assert(index < len);
        return table_raw + index * row_size;
    }
    [[nodiscard]] auto get_row_ptr(size_t index) -> void * {
        assert(index < len);
        return table_raw + index * row_size;
    }
    [[nodiscard]] auto get_component_row_offset(ComponentKey component) const -> size_t {
        size_t res = 0;

        for (ComponentKey c : SignatureView(signature_atlas_ref->get(signature_key))) {
            ComponentData c_data = component_atlas_ref->get(c);

            if (c == component) {
                res = align_up(res, c_data.align);
                break;
            }

            res = align_up(res, c_data.align) + c_data.size;
        }

        return res;
    }
    [[nodiscard]] auto get_component_ptr(size_t index, ComponentKey component) const -> const void * {
        return (const char *)get_row_ptr(index) + get_component_row_offset(component);
    }
    [[nodiscard]] auto get_component_ptr(size_t index, ComponentKey component) -> void * {
        return (char *)get_row_ptr(index) + get_component_row_offset(component);
    }
};

[[nodiscard]] auto align_up(size_t p, size_t align) -> size_t {
    assert((align & (align - 1)) == 0);
    return (p + align - 1) & ~(align - 1);
}

[[nodiscard]] auto handle_cal_row_size(ComponentAtlas *component_atlas_ref, const Signature &signature) -> size_t {
    size_t res = 0;

    for (ComponentKey c : SignatureView(signature)) {
        assert(component_atlas_ref->has(c));

        ComponentData c_data = component_atlas_ref->get(c);

        res = align_up(res, c_data.align) + c_data.size;
    }

    if (signature.any()) {
        ComponentKey c = *SignatureView(signature).begin();

        ComponentData c_data = component_atlas_ref->get(c);

        res = align_up(res, c_data.align);
    }

    return res;
}

} // namespace cactus
