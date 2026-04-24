package ecs

import "base:runtime"
import "core:mem"

ArchetypeTable :: struct {
    row_size:  int,
    table_raw: [^]u8,
    len:       int,
    cap:       int,
    owners:    [dynamic]EntityKey,
    allocator: mem.Allocator,

    // archetype is represented as a table: column = component, row = entity
    // like:
    //           component_0  component_1  component_2
    // entity_0                  ...
    // entity_1    [component data of that entity]
    // entity_2                  ...
    //
    // columns should be fixed during init,
    // so this become more a dynamic array with a whole row as element
}

archetype_table_new :: proc(row_size: int, allocator := context.allocator) -> ArchetypeTable {
    // row_size is supposed to be already align and padding properly
    return ArchetypeTable {
        row_size = row_size,
        table_raw = nil,
        len = 0,
        cap = 0,
        owners = make([dynamic]EntityKey, 0, 0, allocator),
        allocator = allocator,
    }
}

archetype_table_delete :: proc(s: ArchetypeTable) {
    if s.table_raw != nil do mem.free(s.table_raw)
    delete(s.owners)
}

archetype_table_reserve :: proc(s: ^ArchetypeTable, cap: int) {
    if cap <= s.cap do return

    new_table_raw, err := mem.alloc(cap * s.row_size, runtime.DEFAULT_ALIGNMENT, s.allocator)
    if err != .None {
        // TODO error
        assert(false)
        return
    }

    if s.table_raw != nil {
        mem.copy(new_table_raw, s.table_raw, cap * s.row_size)
        free(new_table_raw, s.allocator)
    }

    s.table_raw = cast([^]u8)new_table_raw

    s.cap = cap
}

archetype_table_get_row_ptr :: proc(s: ^ArchetypeTable, index: int) -> ^u8 {
    if index < 0 || index >= s.len do return nil

    return &s.table_raw[index * s.row_size]
}

archetype_table_new_row :: proc(s: ^ArchetypeTable) -> int {
    if s.len >= s.cap {
        new_cap := s.cap + (s.cap >> 1)
        archetype_table_reserve(s, new_cap)
    }

    s.len += 1

    append(&s.owners, EntityKey{})

    return s.len - 1
}

archetype_table_remove_row :: proc(s: ^ArchetypeTable, index: int) -> bool {
    if index < 0 || index >= s.len {
        assert(false)
        return false
    }

    mem.copy(
        rawptr(&s.table_raw[index * s.row_size]),
        rawptr(&s.table_raw[(s.len - 1) * s.row_size]),
        s.row_size,
    )

    s.len -= 1

    unordered_remove_dynamic_array(&s.owners, index)

    return true
}

archetype_table_get_owner :: proc(s: ^ArchetypeTable, index: int) -> (val: EntityKey, ok: bool) {
    if index < 0 || index >= s.len do return {}, false

    return s.owners[index], true
}
