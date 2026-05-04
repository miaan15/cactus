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
    Signature signature;

    char *table_raw;
    size_t row_size;
    size_t len;
    size_t cap;

    [[nodiscard]] static auto make(ComponentAtlas *component_atlas_ref, const Signature &signature) -> Archetype {
        return Archetype{.component_atlas_ref = component_atlas_ref,
                         .signature = signature,
                         .table_raw = nullptr,
                         .row_size = handle_cal_row_size(component_atlas_ref, signature),
                         .len = 0,
                         .cap = 0};
    }

    auto destroy() const { std::free(this->table_raw); }

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
    [[nodiscard]] auto get_component_row_offset(ComponentKey component) const -> size_t {
        size_t res = 0;

        for (ComponentKey c : SignatureView(this->signature)) {
            ComponentData c_data = this->component_atlas_ref->get(c);

            if (c == component) {
                res = align_up(res, c_data.align);
                break;
            }

            res = align_up(res, c_data.align) + c_data.size;
        }

        return res;
    }
    [[nodiscard]] auto get_component_ptr(size_t index, ComponentKey component) const -> const void * {
        return (const char *)this->get_row_ptr(index) + this->get_component_row_offset(component);
    }
    [[nodiscard]] auto get_component_ptr(size_t index, ComponentKey component) -> void * {
        return (char *)this->get_row_ptr(index) + this->get_component_row_offset(component);
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

export using ArchetypeAtlasKey = size_t;

export struct ArchetypeAtlas {
    std::vector<Archetype> archetypes;
    ComponentAtlas *component_atlas_ref;

    [[nodiscard]] static auto make(ComponentAtlas *component_atlas_ref) -> ArchetypeAtlas {
        return ArchetypeAtlas{.archetypes = std::vector<Archetype>(), .component_atlas_ref = component_atlas_ref};
    }

    auto destroy() const {
        for (const auto &a : this->archetypes) a.destroy();
    }

    [[nodiscard]] auto has(ArchetypeAtlasKey key) const -> bool { return key < this->archetypes.size(); }
    [[nodiscard]] auto get(ArchetypeAtlasKey key) const -> Archetype {
        assert(key < this->archetypes.size());
        return this->archetypes[key];
    }
    [[nodiscard]] auto get_ptr(ArchetypeAtlasKey key) -> Archetype * {
        assert(key < this->archetypes.size());
        return &this->archetypes[key];
    }

    auto append(ArchetypeAtlasKey key) -> size_t {
        if (key >= this->archetypes.size()) return -1;
        return this->archetypes[key].append();
    }
    auto remove(ArchetypeAtlasKey key, size_t index) -> bool {
        if (key >= this->archetypes.size()) return false;
        return this->archetypes[key].remove(index);
    }

    auto new_archetype(const Signature &signature) -> size_t {
        this->archetypes.push_back(Archetype::make(this->component_atlas_ref, signature));
        return this->archetypes.size() - 1;
    }
};

} // namespace cactus
