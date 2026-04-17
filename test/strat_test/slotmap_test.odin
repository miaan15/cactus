package strat_test

import "core:testing"

import "../../src/strat"

MyType :: struct {
	x: int,
	y: int,
}

@(test)
slotmap_basic_create_append_get_lencap :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(MyType)
	defer strat.slotmap_delete(&sm)
	key := strat.slotmap_append(&sm, MyType{1, 2})
	value, found := strat.slotmap_get(&sm, key)
	testing.expect(t, found, "Value should be found after insertion")
	testing.expect_value(t, value.x, 1)
	testing.expect_value(t, value.y, 2)
	testing.expect_value(t, strat.slotmap_len(&sm), 1)
	testing.expect(t, strat.slotmap_cap(&sm) >= 1, "Capacity should be >= 1")
}

@(test)
slotmap_append_several_keys_unique_can_get_all :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(MyType)
	defer strat.slotmap_delete(&sm)
	keys: [8]strat.SlotMapKey
	for i in 0 ..< 8 {
		keys[i] = strat.slotmap_append(&sm, MyType{i, i * 10})
	}
	for i in 0 ..< 8 {
		v, ok := strat.slotmap_get(&sm, keys[i])
		testing.expect(t, ok, "Expected key to be found")
		testing.expect_value(t, v.x, i)
		testing.expect_value(t, v.y, i * 10)
	}
	testing.expect_value(t, strat.slotmap_len(&sm), 8)
}

@(test)
slotmap_removal_invalidates_only_removed_key :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(MyType)
	defer strat.slotmap_delete(&sm)
	k1 := strat.slotmap_append(&sm, MyType{11, 11})
	k2 := strat.slotmap_append(&sm, MyType{22, 22})
	_ = strat.slotmap_remove(&sm, k1)
	_, ok := strat.slotmap_get(&sm, k1)
	testing.expect(t, !ok, "Removed key should be invalid")
	v2, ok2 := strat.slotmap_get(&sm, k2)
	testing.expect(t, ok2, "Other keys should remain valid")
	testing.expect_value(t, v2.x, 22)
	testing.expect_value(t, strat.slotmap_len(&sm), 1)
}

@(test)
slotmap_remove_then_reinsert_reuses_slot_bumps_gen :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k1 := strat.slotmap_append(&sm, 42)
	_ = strat.slotmap_remove(&sm, k1)
	k2 := strat.slotmap_append(&sm, 99)
	testing.expect_value(t, k1.index, k2.index)
	testing.expect(t, k1.gen != k2.gen, "Generation should differ after reuse")
	v, ok := strat.slotmap_get(&sm, k2)
	testing.expect(t, ok, "New key must be valid")
	testing.expect_value(t, v, 99)
	_, ok = strat.slotmap_get(&sm, k1)
	testing.expect(t, !ok, "Old key must be invalidated")
}

@(test)
slotmap_stable_handle_across_inserts_and_deletes :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k1 := strat.slotmap_append(&sm, 1)
	_ = strat.slotmap_append(&sm, 2)
	_ = strat.slotmap_append(&sm, 3)
	k_rm := strat.SlotMapKey {
		index = 1,
		gen   = sm.slots[1].gen,
	}
	_ = strat.slotmap_remove(&sm, k_rm)
	v, ok := strat.slotmap_get(&sm, k1)
	testing.expect(t, ok, "Handle must remain stable")
	testing.expect_value(t, v, 1)
	testing.expect_value(t, strat.slotmap_len(&sm), 2)
}

@(test)
slotmap_insert_past_initial_cap_triggers_reserve :: proc(t: ^testing.T) {
	sm := strat.slotmap_new_with_cap(int, 2)
	defer strat.slotmap_delete(&sm)
	keys: [5]strat.SlotMapKey
	for i in 0 ..< 5 {
		keys[i] = strat.slotmap_append(&sm, i * 7)
	}
	for i in 0 ..< 5 {
		v, ok := strat.slotmap_get(&sm, keys[i])
		testing.expect(t, ok, "Expected key to be found after reserve")
		testing.expect_value(t, v, i * 7)
	}
	testing.expect(t, strat.slotmap_cap(&sm) >= 5, "Capacity should have increased")
}

@(test)
slotmap_clear_removes_all_and_invalidates_keys :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	keys: [10]strat.SlotMapKey
	for i in 0 ..< 10 {
		keys[i] = strat.slotmap_append(&sm, i)
	}
	strat.slotmap_clear(&sm)
	testing.expect_value(t, strat.slotmap_len(&sm), 0)
	for i in 0 ..< 10 {
		_, ok := strat.slotmap_get(&sm, keys[i])
		testing.expect(t, !ok, "Cleared key should be invalidated")
	}
}

@(test)
slotmap_reuse_after_clear :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	_ = strat.slotmap_append(&sm, 123)
	strat.slotmap_clear(&sm)
	k := strat.slotmap_append(&sm, 999)
	v, ok := strat.slotmap_get(&sm, k)
	testing.expect(t, ok, "Expect found after reuse")
	testing.expect_value(t, v, 999)
}

@(test)
slotmap_remove_returns_false_for_bogus_key :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k := strat.slotmap_append(&sm, 7)
	bad := strat.SlotMapKey {
		index = 1234,
		gen   = 0,
	}
	testing.expect(t, !strat.slotmap_remove(&sm, bad), "Removing bogus key should fail")
	testing.expect(t, strat.slotmap_remove(&sm, k), "Removing real key should succeed")
	testing.expect(t, !strat.slotmap_remove(&sm, k), "Removing same key again should fail")
}

@(test)
slotmap_get_returns_false_for_invalid_generation :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k1 := strat.slotmap_append(&sm, 10)
	fake := strat.SlotMapKey {
		index = k1.index,
		gen   = k1.gen + 1,
	}
	_, ok := strat.slotmap_get(&sm, fake)
	testing.expect(t, !ok, "Generation-mismatch key must not be found")
}

@(test)
slotmap_get_ptr_and_iterate_ptr_access :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k := strat.slotmap_append(&sm, 100)
	ptr := strat.slotmap_get_ptr(&sm, k)
	testing.expect(t, ptr != nil, "Pointer should not be nil")
	ptr^ = 1234
	v, ok := strat.slotmap_get(&sm, k)
	testing.expect(t, ok, "Expect found after pointer modification")
	testing.expect_value(t, v, 1234)
	idx := 0
	vptr, i, ok_ := strat.slotmap_iterate_ptr(&sm, &idx)
	testing.expect(t, ok_, "First iteration should succeed")
	testing.expect_value(t, vptr^, 1234)
	_, _, ok = strat.slotmap_iterate_ptr(&sm, &idx)
	testing.expect(t, !ok, "Second iteration should fail (size 1)")
}

@(test)
slotmap_iterate_yields_all_in_order :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	vals := []int{2, 4, 8, 16}
	for v in vals {
		_ = strat.slotmap_append(&sm, v)
	}
	idx := 0
	found := []bool{false, false, false, false}
	for {
		v, i, ok := strat.slotmap_iterate(&sm, &idx)
		if !ok {
			break
		}
		testing.expect(t, i < 4, "Index in bounds")
		testing.expect_value(t, v, vals[i])
		found[i] = true
	}
	for f in found {
		testing.expect(t, f, "All values iterated")
	}
}

@(test)
slotmap_reserve_increases_cap_contents_intact :: proc(t: ^testing.T) {
	sm := strat.slotmap_new_with_cap(int, 2)
	defer strat.slotmap_delete(&sm)
	k := strat.slotmap_append(&sm, 77)
	strat.slotmap_reserve(&sm, 100)
	testing.expect(t, strat.slotmap_cap(&sm) >= 100, "Capacity increased after reserve")
	v, ok := strat.slotmap_get(&sm, k)
	testing.expect(t, ok, "Value preserved after reserve")
	testing.expect_value(t, v, 77)
}

@(test)
slotmap_remove_all_and_reuse_slots :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k: [3]strat.SlotMapKey
	k[0] = strat.slotmap_append(&sm, 1)
	k[1] = strat.slotmap_append(&sm, 2)
	k[2] = strat.slotmap_append(&sm, 3)
	testing.expect(t, strat.slotmap_remove(&sm, k[0]), "First remove ok")
	testing.expect(t, strat.slotmap_remove(&sm, k[1]), "Second remove ok")
	testing.expect(t, strat.slotmap_remove(&sm, k[2]), "Third remove ok")
	testing.expect_value(t, strat.slotmap_len(&sm), 0)
	kx := strat.slotmap_append(&sm, 9)
	_, ok := strat.slotmap_get(&sm, kx)
	testing.expect(t, ok, "Slot reuse after all removals")
}

@(test)
slotmap_multiple_remove_insert_sequences_preserve_safety :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	keys: [10]strat.SlotMapKey
	for i in 0 ..< 10 {
		keys[i] = strat.slotmap_append(&sm, i)
	}
	for i in 0 ..< 10 {
		if i % 2 == 0 {
			testing.expect(t, strat.slotmap_remove(&sm, keys[i]), "Remove on even")
		}
	}
	for i in 0 ..< 10 {
		if i % 2 == 0 {
			k := strat.slotmap_append(&sm, i + 100)
			_, ok := strat.slotmap_get(&sm, k)
			testing.expect(t, ok, "Insert after remove")
		}
	}
	for i in 1 ..< 10 {
		if i % 2 != 0 {
			_, ok := strat.slotmap_get(&sm, keys[i])
			testing.expect(t, ok, "Odd keys remain")
		}
	}
}

@(test)
slotmap_modify_via_ptr_after_mutation :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(MyType)
	defer strat.slotmap_delete(&sm)
	keys: [10]strat.SlotMapKey
	for i in 0 ..< 10 {
		keys[i] = strat.slotmap_append(&sm, MyType{i, 0})
	}
	_ = strat.slotmap_remove(&sm, keys[5])
	ptr := strat.slotmap_get_ptr(&sm, keys[6])
	testing.expect(t, ptr != nil, "Pointer valid after removes")
	ptr.y = 42
	v, ok := strat.slotmap_get(&sm, keys[6])
	testing.expect(t, ok, "Should find after direct pointer edit")
	testing.expect_value(t, v.y, 42)
}

@(test)
slotmap_removing_last_element :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k1 := strat.slotmap_append(&sm, 1)
	k2 := strat.slotmap_append(&sm, 2)
	_ = strat.slotmap_remove(&sm, k2)
	v, ok := strat.slotmap_get(&sm, k1)
	testing.expect(t, ok, "First key valid after removing last")
	testing.expect_value(t, v, 1)
	_ = strat.slotmap_remove(&sm, k1)
	testing.expect_value(t, strat.slotmap_len(&sm), 0)
}

@(test)
slotmap_handles_empty_case_gracefully :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	key := strat.SlotMapKey {
		index = 0,
		gen   = 0,
	}
	testing.expect(t, !strat.slotmap_remove(&sm, key), "Remove on empty returns false")
	_, ok := strat.slotmap_get(&sm, key)
	testing.expect(t, !ok, "Get on empty returns false")
}

@(test)
slotmap_reappend_after_full_removal_and_clear :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k := strat.slotmap_append(&sm, 1)
	testing.expect(t, strat.slotmap_remove(&sm, k), "Remove works")
	strat.slotmap_clear(&sm)
	nkey := strat.slotmap_append(&sm, 99)
	v, ok := strat.slotmap_get(&sm, nkey)
	testing.expect(t, ok, "Valid after re-append post-clear")
	testing.expect_value(t, v, 99)
}

@(test)
slotmap_can_remove_reinsert_same_slot_repeatedly :: proc(t: ^testing.T) {
	sm := strat.slotmap_new(int)
	defer strat.slotmap_delete(&sm)
	k := strat.slotmap_append(&sm, 7)
	for i in 0 ..< 10 {
		_ = strat.slotmap_remove(&sm, k)
		k = strat.slotmap_append(&sm, i)
		v, ok := strat.slotmap_get(&sm, k)
		testing.expect(t, ok, "Get after repeated remove/insert")
		testing.expect_value(t, v, i)
	}
}
