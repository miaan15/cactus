package ecs

import "core:mem"
import "core:slice"
import "core:strings"

ComponentData :: struct {
    size, align: int,
}

ComponentAtlas :: struct {
    cpn_data_map: map[typeid]ComponentData,
    allocator:    mem.Allocator,
}

SignatureData :: string // Signature is a string that represent list of component
// Consider SignatureData as a list of typeid that to access the ComponentAtlas:cpn_property_map
// to get specific component's size and alignment

SignatureKey :: distinct int

SignatureCacheTransition :: struct {
    from: SignatureKey,
    cpn:  typeid,
}

SignatureAtlas :: struct {
    signature_datas:            [dynamic]SignatureData,
    data_to_key_map:            map[SignatureData]SignatureKey,
    signature_add_cpn_cache:    map[SignatureCacheTransition]SignatureKey,
    signature_remove_cpn_cache: map[SignatureCacheTransition]SignatureKey,
    allocator:                  mem.Allocator,

    // NOTE signature_datas datas should outlive data_to_key_map

    // This is to tell if a signature is added or removed component then it will become what
}

EMPTY_SIGNATURE_KEY: SignatureKey : -1

component_atlas_new :: proc(allocator := context.allocator) -> ComponentAtlas {
    return ComponentAtlas {
        cpn_data_map = make(map[typeid]ComponentData, allocator),
        allocator = allocator,
    }
}

component_atlas_delete :: proc(s: ComponentAtlas) {
    delete(s.cpn_data_map)
}

component_atlas_has_component :: proc(s: ComponentAtlas, component: typeid) -> bool {
    _, ok := s.cpn_data_map[component]
    return ok
}

component_atlas_register_component :: proc(s: ^ComponentAtlas, $T: typeid) {
    s.cpn_data_map[typeid_of(T)] = ComponentData {
        size  = size_of(T),
        align = align_of(T),
    }
}

component_atlas_get_component_data_typeid :: proc(
    s: ComponentAtlas,
    cpn: typeid,
) -> (
    val: ComponentData,
    ok: bool,
) {
    return s.cpn_data_map[cpn]
}
component_atlas_get_component_data_uintptr :: proc(
    s: ComponentAtlas,
    cpn_uintptr: uintptr,
) -> (
    val: ComponentData,
    ok: bool,
) {
    return s.cpn_data_map[transmute(typeid)cpn_uintptr]
}

component_atlas_get_component_data :: proc {
    component_atlas_get_component_data_typeid,
    component_atlas_get_component_data_uintptr,
}


signature_data_new :: proc() -> SignatureData {
    return {}
}

signature_data_delete :: proc(s: SignatureData) {
    delete(s)
}

signature_data_has_component :: proc(s: SignatureData, component: typeid) -> bool {
    str_len := len(s)
    cpn_data_slice := slice.from_ptr(transmute(^uintptr)raw_data(s), str_len / size_of(uintptr))
    for cpn_uintptr in cpn_data_slice {
        if cpn_uintptr == transmute(uintptr)component do return true
    }
    return false
}

signature_data_add_component :: proc(s: ^SignatureData, component: typeid) {
    // cast component typeid to uintptr and then to []u8 then put all into a string
    cpn_uintptr := transmute(uintptr)component
    cpn_uintptr_byte_slice := slice.from_ptr(transmute(^u8)(&cpn_uintptr), size_of(uintptr))

    b: strings.Builder
    strings.builder_init(&b)
    defer strings.builder_destroy(&b)

    strings.write_string(&b, s^)
    strings.write_bytes(&b, cpn_uintptr_byte_slice)

    delete(s^)
    s^ = strings.to_string(b)
}

signature_data_remove_component :: proc(s: ^SignatureData, component: typeid) -> bool {
    // cast component typeid to uintptr then find and remove it from the string
    str_len := len(s^)
    cpn_data_slice := slice.from_ptr(transmute(^uintptr)raw_data(s^), str_len / size_of(uintptr))
    for cpn_uintptr, index in cpn_data_slice {
        if cpn_uintptr == transmute(uintptr)component {
            prefix := s^[:index * size_of(uintptr)]
            suffix := s^[(index + 1) * size_of(uintptr):]

            delete(s^)
            s^ = strings.concatenate({prefix, suffix})

            return true
        }
    }

    assert(false)
    return false
}

signature_data_get_component :: proc(s: SignatureData, index: int) -> (val: typeid, ok: bool) {
    len := len(s) / size_of(uintptr)
    if index < 0 || index >= len do return {}, false

    cpn_data_slice := slice.from_ptr(transmute(^typeid)raw_data(s), len)

    return cpn_data_slice[index], true
}

signature_data_get_component_slice :: proc(s: SignatureData) -> []typeid {
    return slice.from_ptr(transmute(^typeid)raw_data(s), len(s) / size_of(uintptr))
}

signature_data_len :: proc(s: SignatureData) -> int {
    return len(s) / size_of(uintptr)
}

signature_atlas_new :: proc(allocator := context.allocator) -> SignatureAtlas {
    return SignatureAtlas {
        signature_datas = make([dynamic]SignatureData, allocator),
        data_to_key_map = make(map[SignatureData]SignatureKey, allocator),
        signature_add_cpn_cache = make(map[SignatureCacheTransition]SignatureKey, allocator),
        signature_remove_cpn_cache = make(map[SignatureCacheTransition]SignatureKey, allocator),
        allocator = allocator,
    }
}

signature_atlas_delete :: proc(s: SignatureAtlas) {
    delete(s.data_to_key_map)
    delete(s.signature_datas)
    delete(s.signature_add_cpn_cache)
    delete(s.signature_remove_cpn_cache)
}

signature_atlas_get_data :: proc(
    s: SignatureAtlas,
    key: SignatureKey,
) -> (
    val: SignatureData,
    ok: bool,
) {
    if int(key) < 0 && int(key) >= len(s.signature_datas) do return {}, false
    return s.signature_datas[key], true
}

signature_atlas_add_component :: proc(
    s: ^SignatureAtlas,
    from_signature: SignatureKey,
    component: typeid,
) -> (
    val: SignatureKey,
    ok: bool,
) {
    if int(from_signature) >= 0 && int(from_signature) < len(s.signature_datas) {
        assert(false)
        return 0, false
    }
    if from_signature == EMPTY_SIGNATURE_KEY ||
       !signature_data_has_component(s.signature_datas[int(from_signature)], component) {
        assert(false)
        return 0, false
    }

    // if existed in cached transition
    if to_signature, ok :=
           s.signature_add_cpn_cache[SignatureCacheTransition{from_signature, component}]; ok {
        return to_signature, true
    }

    // if the from is just empty then create new signature
    if (from_signature == EMPTY_SIGNATURE_KEY) {
        new_signature := signature_data_new()
        signature_data_add_component(&new_signature, component)
        append(&s.signature_datas, new_signature)

        to_signature := SignatureKey(len(s.signature_datas) - 1)
        s.signature_add_cpn_cache[SignatureCacheTransition{from_signature, component}] =
            to_signature
        s.signature_remove_cpn_cache[SignatureCacheTransition{to_signature, component}] =
            from_signature

        return to_signature, true
    }

    // create a temp data for compare and assign new
    old_data := &s.signature_datas[int(from_signature)]
    temp_new_signature_data := strings.clone(old_data^)
    signature_data_add_component(&temp_new_signature_data, component)

    to_signature: SignatureKey
    if temp_new_signature_key, ok := s.data_to_key_map[temp_new_signature_data]; ok {
        delete(temp_new_signature_data)
        to_signature = temp_new_signature_key
    } else {
        append(&s.signature_datas, temp_new_signature_data)

        to_signature = SignatureKey(len(s.signature_datas) - 1)

        s.data_to_key_map[temp_new_signature_data] = to_signature
        // this might be dangerous since signature_datas and data_to_key_map use the same string
        // but expected the lifetime of both are the same
    }

    s.signature_add_cpn_cache[SignatureCacheTransition{from_signature, component}] = to_signature

    return to_signature, true
}

signature_atlas_remove_component :: proc(
    s: ^SignatureAtlas,
    from_signature: SignatureKey,
    component: typeid,
) -> (
    val: SignatureKey,
    ok: bool,
) {
    if int(from_signature) >= 0 && int(from_signature) < len(s.signature_datas) {
        assert(false)
        return 0, false
    }
    if from_signature != EMPTY_SIGNATURE_KEY &&
       signature_data_has_component(s.signature_datas[int(from_signature)], component) {
        assert(false)
        return 0, false
    }

    // if existed in cached transition
    if to_signature, ok :=
           s.signature_remove_cpn_cache[SignatureCacheTransition{from_signature, component}]; ok {
        return to_signature, true
    }

    // create a temp data for compare and assign new
    old_data := &s.signature_datas[int(from_signature)]
    temp_new_signature_data := strings.clone(old_data^)
    if !signature_data_remove_component(&temp_new_signature_data, component) {
        assert(false)
        return 0, false
    }

    to_signature: SignatureKey
    if temp_new_signature_key, ok := s.data_to_key_map[temp_new_signature_data]; ok {
        delete(temp_new_signature_data)
        to_signature = temp_new_signature_key
    } else {
        append(&s.signature_datas, temp_new_signature_data)

        to_signature = SignatureKey(len(s.signature_datas) - 1)

        s.data_to_key_map[temp_new_signature_data] = to_signature
        // this might be dangerous since signature_datas and data_to_key_map use the same string
        // but expected the lifetime of both are the same
    }

    s.signature_remove_cpn_cache[SignatureCacheTransition{from_signature, component}] =
        to_signature

    return to_signature, true
}
