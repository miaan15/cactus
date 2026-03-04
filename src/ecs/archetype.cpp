module;

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

export module Ecs:Archetype;

import :Common;

namespace cactus::ecs {

export constexpr auto align_up_offset(size_t offset, size_t alignment) -> size_t {
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    return (offset + alignment - 1) & ~(alignment - 1);
}

export struct ArchetypeTable {
    void *table_ptr;
    Entity *entity_owned_list_ptr;

    size_t row_count = 0;
    size_t capacity = 0;
    size_t row_size = 0;
    size_t row_align = 0;

    constexpr auto free() {
        ::free(table_ptr);
        ::free(entity_owned_list_ptr);
    }

    [[nodiscard]] auto get_row_ptr(size_t row) -> void * {
        assert(row_size > 0);
        assert(row_align > 0 && (row_align & (row_align - 1)) == 0);
        assert(row < row_count);
        auto row_size_with_padding = align_up_offset(row_size, row_align);
        return (std::byte *)table_ptr + (row * row_size_with_padding);
    }

    [[nodiscard]] auto create_row() -> size_t {
        assert(row_size > 0);
        assert(row_align > 0 && (row_align & (row_align - 1)) == 0);

        auto row_size_with_padding = align_up_offset(row_size, row_align);

        if (row_count >= capacity) {
            size_t new_capacity = capacity == 0 ? 8 : capacity + (capacity >> 1);

            size_t effective_align = std::max(row_align, alignof(std::max_align_t));
            size_t total_size = new_capacity * row_size_with_padding;
            total_size = align_up_offset(total_size, effective_align);

            void *new_table_ptr = aligned_alloc(effective_align, total_size);
            assert(new_table_ptr);
            if (table_ptr) {
                memcpy(new_table_ptr, table_ptr, capacity * row_size_with_padding);
            }
            table_ptr = new_table_ptr;

            auto *new_entity_list = (Entity *)malloc(new_capacity * sizeof(Entity));
            if (entity_owned_list_ptr) {
                memcpy(new_entity_list, entity_owned_list_ptr, capacity * sizeof(Entity));
            }
            entity_owned_list_ptr = new_entity_list;

            capacity = new_capacity;
        }

        memset((std::byte *)table_ptr + (row_count * row_size_with_padding), 0,
               row_size_with_padding);
        entity_owned_list_ptr[row_count] = {};

        ++row_count;

        return row_count - 1;
    }

    auto remove_row(size_t row) {
        assert(row_size > 0);
        assert(row_align > 0 && (row_align & (row_align - 1)) == 0);
        assert(row < row_count);

        if (row != row_count - 1) {
            auto row_size_with_padding = align_up_offset(row_size, row_align);

            memcpy(get_row_ptr(row), get_row_ptr(row_count - 1), row_size_with_padding);

            entity_owned_list_ptr[row] = entity_owned_list_ptr[row_count - 1];
        }

        --row_count;
    }
};

} // namespace cactus::ecs
