module;

export module cact.core.ecs:table;

import :defines;
import :utils;
import cact.common;
import cact.core.strat;

namespace cact::detail::ecs {

export struct Table {
    char *table_raw = nullptr;
    Entity *owner_list_raw = nullptr;
    size_t row_size;
    size_t len = 0;
    size_t cap = 0;

    Signature signature;

    FixedArray<size_t> component_offset_list;

    [[nodiscard]] static auto make(Signature signature, const FixedArray<ComponentData> &component_data_list) noexcept -> Table {
        size_t offset = 0;
        size_t max_align = 1;

        auto component_offset_list = FixedArray<size_t>::make(component_data_list.len);
        for (auto signature_ull = signature.to_ullong(); signature_ull > 0; signature_ull &= (signature_ull - 1)) {
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
    auto destroy() {
        if (table_raw) std::free(table_raw);
        if (owner_list_raw) std::free(owner_list_raw);
        component_offset_list.destroy();
    }
    [[nodiscard]] auto clone() noexcept -> Table = delete; // TODO

    [[nodiscard]] auto get_row_ptr(size_t row_index) const noexcept -> const void * {
        _assert(row_index < len, "Row index out of bounds");
        return table_raw + row_index * row_size;
    }
    [[nodiscard]] auto get_row_ptr(size_t row_index) noexcept -> void * {
        _assert(row_index < len, "Row index out of bounds");
        return table_raw + row_index * row_size;
    }

    [[nodiscard]] auto get_component_offset(size_t component_index) const noexcept -> size_t {
        _assert(component_index < component_offset_list.len, "Component index out of bounds");
        return component_offset_list.get(component_index).value();
    }

    [[nodiscard]] auto get_component_ptr(size_t row_index, size_t component_index) const noexcept -> const void * {
        return (const char *)get_row_ptr(row_index) + get_component_offset(component_index);
    }
    [[nodiscard]] auto get_component_ptr(size_t row_index, size_t component_index) noexcept -> void * {
        return (char *)get_row_ptr(row_index) + get_component_offset(component_index);
    }

    auto reserve(size_t new_cap) {
        if (new_cap <= cap) return;

        auto *new_table_raw = (char *)std::realloc(table_raw, new_cap * row_size);
        _assert(new_table_raw, "Failed to reallocate table_raw");

        auto *new_owner_list_raw = (Entity *)std::realloc(owner_list_raw, new_cap * sizeof(Entity));
        _assert(new_owner_list_raw, "Failed to reallocate owner_list_raw");

        table_raw = new_table_raw;
        owner_list_raw = new_owner_list_raw;
        cap = new_cap;
    }

    auto new_row(Entity entity_owner) noexcept -> size_t {
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
    auto remove_row(size_t index) noexcept -> std::optional<Entity> {
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

} // namespace cact::detail::ecs
