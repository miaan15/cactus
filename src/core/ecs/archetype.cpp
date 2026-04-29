module;

#include <cassert>

export module cactus.core.ecs:archetype;

import :component;
import :signature;

import std;

using size_t = std::size_t;

namespace cactus {

export [[nodiscard]] auto align_up(size_t p, size_t align) -> size_t;
[[nodiscard]] auto handle_cal_row_size(ComponentSizeAlignAtlas *component_size_align_atlas_ref,
                                       SignatureAtlas *signature_atlas_ref, SignatureAtlasKey signature_key) -> size_t;

export struct Archetype {
    ComponentSizeAlignAtlas *component_size_align_atlas_ref;
    SignatureAtlas *signature_atlas_ref;
    SignatureAtlasKey signature_key;

    char *table_raw;
    size_t row_size;
    size_t len;
    size_t cap;

    [[nodiscard]] static auto make(ComponentSizeAlignAtlas *component_size_align_atlas_ref, SignatureAtlas *signature_atlas_ref,
                                   SignatureAtlasKey signature_key) -> Archetype {
        assert(signature_atlas_ref->has(signature_key));

        return Archetype{.component_size_align_atlas_ref = component_size_align_atlas_ref,
                         .signature_atlas_ref = signature_atlas_ref,
                         .signature_key = signature_key,
                         .table_raw = nullptr,
                         .row_size = handle_cal_row_size(component_size_align_atlas_ref, signature_atlas_ref, signature_key),
                         .len = 0,
                         .cap = 0};
    }

    auto destroy() const { std::free(table_raw); }

    auto reserve(size_t cap) {
        if (cap <= this->cap) return;

        char *new_table_raw = (char *)std::malloc(cap * this->row_size);

        if (this->table_raw != nullptr) {
            std::memcpy(new_table_raw, this->table_raw, this->len * this->row_size);
            std::free(this->table_raw);
        }

        this->table_raw = new_table_raw;
        this->cap = cap;
    }
    auto append() -> size_t {
        if (this->len >= this->cap) {
            size_t new_cap = this->cap * 2;
            if (new_cap < 4) new_cap = 4;
            this->reserve(new_cap);
        }

        ++this->len;

        char *last_row_ptr = this->table_raw + (this->len - 1) * this->row_size;
        std::memset(last_row_ptr, 0, this->row_size);

        return this->len - 1;
    }
    auto remove(size_t index) -> bool {
        if (index >= this->len) return false;

        char *t_row_ptr = this->table_raw + index * this->row_size;
        char *last_row_ptr = this->table_raw + (this->len - 1) * this->row_size;
        std::memcpy(t_row_ptr, last_row_ptr, this->row_size);

        --this->len;

        return true;
    }

    [[nodiscard]] auto get_row_ptr(size_t index) const -> const void * {
        assert(index < this->len);
        return this->table_raw + index * this->row_size;
    }
    [[nodiscard]] auto get_row_ptr(size_t index) -> void * {
        assert(index < this->len);
        return this->table_raw + index * this->row_size;
    }
    [[nodiscard]] auto get_component_row_offset(std::type_index component) const -> size_t {
        Signature signature = signature_atlas_ref->get(signature_key);
        assert(signature.has(component));

        size_t res = 0;

        for (size_t i = 0; i < signature.len; i++) {
            std::type_index c = signature.data_raw[i];

            assert(component_size_align_atlas_ref->has(c));
            auto size_align_data = component_size_align_atlas_ref->get_size_align(c);
            size_t c_size = size_align_data.size;
            size_t c_align = size_align_data.align;

            if (c == component) {
                res = align_up(res, c_align);
                break;
            }

            res = align_up(res, c_align) + c_size;
        }

        return res;
    }
    [[nodiscard]] auto get_component_ptr(size_t index, std::type_index component) const -> const void * {
        assert(index < this->len);
        Signature signature = signature_atlas_ref->get(signature_key);
        assert(signature.has(component));

        return (const char *)this->get_row_ptr(index) + this->get_component_row_offset(component);
    }
    [[nodiscard]] auto get_component_ptr(size_t index, std::type_index component) -> void * {
        assert(index < this->len);
        Signature signature = signature_atlas_ref->get(signature_key);
        assert(signature.has(component));

        return (char *)this->get_row_ptr(index) + this->get_component_row_offset(component);
    }
};

[[nodiscard]] auto align_up(size_t p, size_t align) -> size_t {
    assert((align & (align - 1)) == 0);
    return (p + align - 1) & ~(align - 1);
}

[[nodiscard]] auto handle_cal_row_size(ComponentSizeAlignAtlas *component_size_align_atlas_ref,
                                       SignatureAtlas *signature_atlas_ref, SignatureAtlasKey signature_key) -> size_t {
    Signature signature = signature_atlas_ref->get(signature_key);

    size_t res = 0;

    for (size_t i = 0; i < signature.len; i++) {
        std::type_index c = signature.data_raw[i];

        assert(component_size_align_atlas_ref->has(c));
        auto size_align_data = component_size_align_atlas_ref->get_size_align(c);
        size_t c_size = size_align_data.size;
        size_t c_align = size_align_data.align;

        res = align_up(res, c_align) + c_size;
    }

    if (signature.len > 1) {
        std::type_index c = signature.data_raw[0];

        auto size_align_data = component_size_align_atlas_ref->get_size_align(c);
        size_t c_size = size_align_data.size;
        size_t c_align = size_align_data.align;

        res = align_up(res, c_align);
    }

    return res;
}

export using ArchetypeAtlasKey = size_t;

export struct ArchetypeAtlas {
    std::vector<Archetype> archetypes;
    ComponentSizeAlignAtlas *component_size_align_atlas_ref;
    SignatureAtlas *signature_atlas_ref;

    [[nodiscard]] static auto make(ComponentSizeAlignAtlas *component_size_align_atlas_ref, SignatureAtlas *signature_atlas_ref)
        -> ArchetypeAtlas {
        return ArchetypeAtlas{.archetypes = {},
                              .component_size_align_atlas_ref = component_size_align_atlas_ref,
                              .signature_atlas_ref = signature_atlas_ref};
    }

    auto destroy() const {
        for (const auto &a : archetypes) a.destroy();
    }

    [[nodiscard]] auto has(ArchetypeAtlasKey key) const -> bool { return key < archetypes.size(); }
    [[nodiscard]] auto get(ArchetypeAtlasKey key) const -> Archetype {
        assert(key < archetypes.size());
        return archetypes[key];
    }
    [[nodiscard]] auto get_ptr(ArchetypeAtlasKey key) -> Archetype * {
        assert(key < archetypes.size());
        return &archetypes[key];
    }

    auto append(ArchetypeAtlasKey key) -> size_t {
        if (key >= archetypes.size()) return -1;
        return archetypes[key].append();
    }
    auto remove(ArchetypeAtlasKey key, size_t index) -> bool {
        if (key >= archetypes.size()) return false;
        return archetypes[key].remove(index);
    }

    auto new_archetype(SignatureAtlasKey signature_key) -> size_t {
        archetypes.push_back(Archetype::make(this->component_size_align_atlas_ref, this->signature_atlas_ref, signature_key));
        return archetypes.size() - 1;
    }
};

} // namespace cactus
