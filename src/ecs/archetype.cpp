module;

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>

export module Ecs:Archetype;

import :Common;

namespace cactus::ecs {

export struct ArchetypeTable {
    void *table_ptr;
    Entity *entity_owned_list_ptr;

    size_t row_count;
    size_t capacity;
    size_t row_size;

    ArchetypeTable(size_t row_size)
        : table_ptr(nullptr), entity_owned_list_ptr(nullptr), row_count(0), capacity(0),
          row_size(row_size) {}
    ArchetypeTable(const ArchetypeTable &) = delete;
    ArchetypeTable &operator=(const ArchetypeTable &) = delete;
    ArchetypeTable(ArchetypeTable &&) noexcept = default;
    ArchetypeTable &operator=(ArchetypeTable &&) noexcept = default;
    ~ArchetypeTable() {
        free(table_ptr);
        free(entity_owned_list_ptr);
    }

    [[nodiscard]] auto create_row() -> size_t {
        if (row_count >= capacity) {
            size_t new_capacity = capacity;
            if (capacity == 0) {
                new_capacity = 8;

                auto *new_table_ptr = malloc(new_capacity * row_size);
                assert(new_table_ptr);
                table_ptr = new_table_ptr;

                auto *new_entity_list = (Entity *)malloc(new_capacity * sizeof(Entity));
                assert(new_entity_list);
                entity_owned_list_ptr = new_entity_list;
            } else {
                new_capacity = capacity + (capacity >> 1);

                auto *new_table_ptr = realloc(table_ptr, new_capacity * row_size);
                assert(new_table_ptr);
                table_ptr = new_table_ptr;

                auto *new_entity_list =
                    (Entity *)realloc(entity_owned_list_ptr, new_capacity * sizeof(Entity));
                assert(new_entity_list);
                entity_owned_list_ptr = new_entity_list;
            }
            capacity = new_capacity;
        }
        memset((std::byte *)table_ptr + (row_count * row_size), 0, row_size);

        entity_owned_list_ptr[row_count] = Entity{0};

        return row_count++;
    }

    [[nodiscard]] auto get_row_ptr(size_t row) -> void * {
        assert(row < row_count);
        return (std::byte *)table_ptr + (row * row_size);
    }
};

} // namespace cactus::ecs
