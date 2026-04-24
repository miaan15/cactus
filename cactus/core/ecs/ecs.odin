package ecs

import "cactus:core/strat"
import "core:mem"

EntityKey :: strat.SlotMapKey

EntityWorldData :: struct {
    signature_key: SignatureKey,
    row_index:     int,
}

World :: struct {
    entity_datas:     strat.SlotMap(EntityWorldData),
    archetype_tables: map[SignatureKey]ArchetypeTable,
    component_atlas:  ComponentAtlas,
    signature_atlas:  SignatureAtlas,
    allocator:        mem.Allocator,

    // World has a list of all its entity,
    // each entity has its Signature to know what components it has,
    // and by Signature it has the Archetype by archetype_tables and row_index to get the exact data it holds
    // component_atlas tells the size and aligment of each component
    // signature_atlas tells what each Signature represent
}

world_new :: proc(allocator := context.allocator) -> World {
    return World {
        entity_datas = strat.slotmap_new(EntityWorldData, allocator),
        archetype_tables = make(map[SignatureKey]ArchetypeTable, allocator),
        component_atlas = component_atlas_new(allocator),
        signature_atlas = signature_atlas_new(allocator),
        allocator = allocator,
    }
}

world_delete :: proc(s: ^World) {
    strat.slotmap_delete(s.entity_datas)
    delete(s.archetype_tables)
    component_atlas_delete(s.component_atlas)
    signature_atlas_delete(s.signature_atlas)
}

world_create_entity :: proc(s: ^World) -> EntityKey {
    return strat.slotmap_append(&s.entity_datas, EntityWorldData{EMPTY_SIGNATURE_KEY, 0})
}

world_has_entity :: proc(s: ^World, key: EntityKey) -> bool {
    return strat.slotmap_has(&s.entity_datas, key)
}

world_entity_get_component :: proc(
    s: ^World,
    entity: EntityKey,
    $T: typeid,
) -> (
    val: T,
    ok: bool,
) {
    if !world_has_entity(s, entity) do return {}, false
    if !component_atlas_has_component(s.component_atlas, typeid_of(T)) do return {}, false

    entity_data := strat.slotmap_get(s.entity_datas, entity) or_return
    table := s.archetype_tables[entity_data.signature_key] or_return

    return transmute(^T)world_archetype_component_data_ptr(s, &table, entity_data, typeid_of(T))
}

world_entity_has_component :: proc(s: ^World, entity: EntityKey, $T: typeid) -> bool {
    if !world_has_entity(s, entity) do return false
    if !component_atlas_has_component(s.component_atlas, typeid_of(T)) do return false

    entity_data := strat.slotmap_get(s.entity_datas, entity) or_return

    return signature_data_has_component(
        signature_atlas_get_data(entity_data.signature_key),
        typeid_of(T),
    )
}

world_entity_add_component :: proc(s: ^World, entity: EntityKey, $T: typeid, val: T) -> bool {
    if !world_has_entity(s, entity) do return false

    if !component_atlas_has_component(s.component_atlas, typeid_of(T)) {
        component_atlas_register_component(&s.component_atlas, T)
    }

    entity_data := strat.slotmap_get(s.entity_datas, entity)
    signature_key := entity_data.signature_key

    // if that component existed, just overwrite
    if world_entity_has_component(s, entity, T) {
        table := s.archetype_tables[signature_key]
        component_ptr := transmute(^T)world_archetype_component_data_ptr(
            s,
            &table,
            entity_data,
            typeid_of(T),
        )
        mem.copy(component_ptr, &val, size_of(T))

        return
    }

    new_signature_key := signature_atlas_add_component(
        &s.signature_atlas,
        signature_key,
        typeid_of(T),
    )

    new_table, is_already_existed_new_table := s.archetype_tables[new_signature_key]
    if !is_already_existed_new_table {
        new_table.row_size = world_signature_row_size(s^, new_signature_key)
    }

    new_row := archetype_table_new_row(&new_table)
    new_row_ptr := archetype_table_get_row_ptr(&new_table, new_row)

    // if entity does not have component yet and entity does not exist any data
    if (signature_key == EMPTY_SIGNATURE_KEY) {
        mem.copy(new_row_ptr, val, size_of(T))

        new_table.owners[new_row] = entity

        s.entity_datas[entity].signature_key = new_signature_key
        s.entity_datas[entity].row_index = new_row

        return
    }

    // move entity from old archetype table to new one
    old_row := s.entity_datas[entity].row_index


}

world_entity_remove_component :: proc(s: ^World, entity: EntityKey, $T: typeid) -> bool {
    if !world_has_entity(s, entity) do return false

    if !component_atlas_has_component(s.component_atlas, typeid_of(T)) do return false

    entity_data := strat.slotmap_get(s.entity_datas, entity)
    signature_key := entity_data.signature_key

    if world_entity_has_component(s, entity, T) {

    }
}

world_archetype_component_data_ptr :: proc(
    s: ^World,
    table: ^ArchetypeTable,
    entity_data: EntityWorldData,
    component: typeid,
) -> ^u8 {
    row_ptr := &table.table_raw[table.row_size * entity_data.row_index]

    offset, ok := world_signature_component_row_offset(s^, entity_data.signature_key, component)
    if !ok do return nil
    final_ptr := mem.ptr_offset(row_ptr, offset)

    return final_ptr
}

world_signature_row_size :: proc(s: World, signature: SignatureKey) -> (val: int, ok: bool) {
    assert(int(signature) >= 0 && int(signature) < len(s.signature_atlas.signature_datas))

    s := s

    total: int = 0
    signature_data := signature_atlas_get_data(s.signature_atlas, signature) or_return
    cpns_slice := signature_data_get_component_slice(signature_data)
    for cpn in cpns_slice {
        cpn_data := component_atlas_get_component_data(s.component_atlas, cpn) or_return

        total = align_up_offset(total, cpn_data.align) + cpn_data.size
    }
    if len(cpns_slice) > 1 {
        cpn := signature_data_get_component(signature_data, 0) or_return
        component_data := component_atlas_get_component_data(s.component_atlas, cpn) or_return

        total = align_up_offset(total, component_data.align)
    }
    return total, true
}

world_signature_component_row_offset :: proc(
    s: World,
    signature: SignatureKey,
    component: typeid,
) -> (
    val: int,
    ok: bool,
) {
    assert(int(signature) >= 0 && int(signature) < len(s.signature_atlas.signature_datas))
    assert(component_atlas_has_component(s.component_atlas, component))

    total: int = 0
    signature_data := signature_atlas_get_data(s.signature_atlas, signature) or_return
    cpns_slice := signature_data_get_component_slice(signature_data)
    flag := false
    for cpn in cpns_slice {
        cpn_data := component_atlas_get_component_data(s.component_atlas, cpn) or_return
        if cpn == component {
            flag := true
            break
        }

        total = align_up_offset(total, cpn_data.align) + cpn_data.size
    }

    if !flag do return {}, false

    return total, true
}

archetype_copy_data :: proc(
    component_atlas: ComponentAtlas,
    from_table: ArchetypeTable,
    from_row: int,
    from_signature: SignatureData,
    to_table: ^ArchetypeTable,
    to_row: int,
    to_signature: SignatureData,
) {
    from_table := from_table
    from_row_ptr := archetype_table_get_row_ptr(&from_table, from_row)
    to_row_ptr := archetype_table_get_row_ptr(to_table, to_row)

    from_signature_slice := signature_data_get_component_slice(from_signature)
    to_signature_slice := signature_data_get_component_slice(to_signature)

    from_component_offset, to_component_offset: int = 0, 0

    for i, j: int; i < len(from_signature_slice) && j < len(to_signature_slice); i += 1 {
        for from_signature_slice[i] != to_signature_slice[j] && j < len(to_signature_slice) {
            j += 1

            cpn_data, ok := component_atlas_get_component_data(
                component_atlas,
                from_signature_slice[i],
            )
            if !ok do return

            to_component_offset += cpn_data.size
            align_up_offset(to_component_offset, cpn_data.size)
        }
        if j < len(to_signature_slice) do break

        cpn_data, ok := component_atlas_get_component_data(
            component_atlas,
            from_signature_slice[i],
        )
        if !ok do return

        mem.copy(
            mem.ptr_offset(from_row_ptr, from_component_offset),
            mem.ptr_offset(to_row_ptr, to_component_offset),
            cpn_data.size,
        )

        from_component_offset += cpn_data.size
        align_up_offset(from_component_offset, cpn_data.size)

        to_component_offset += cpn_data.size
        align_up_offset(to_component_offset, cpn_data.size)
    }

}

align_up_offset :: proc(offset, align: int) -> int {
    assert(align > 0 && (align & (align - 1)) == 0)
    return (offset + align - 1) & ~(align - 1)
}
