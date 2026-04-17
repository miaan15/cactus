package strat

import "core:mem"

SlotMapKey :: struct {
	index: int,
	gen:   int,
}

SlotMap :: struct($T: typeid) {
	data_raw:            [^]T,
	data_slot_index_raw: [^]int,
	len:                 int,
	cap:                 int,
	slots:               [dynamic]SlotMapKey,
	next_slot_index:     int,
	allocator:           mem.Allocator,
}

slotmap_new_no_cap :: proc($T: typeid, allocator := context.allocator) -> SlotMap(T) {
	return SlotMap(T) {
		data_raw = nil,
		data_slot_index_raw = nil,
		len = 0,
		cap = 0,
		slots = make([dynamic]SlotMapKey, 0, 0, allocator),
		next_slot_index = 0,
		allocator = allocator,
	}
}
slotmap_new_with_cap :: proc($T: typeid, cap: int, allocator := context.allocator) -> SlotMap(T) {
	data_raw, err := mem.alloc(cap * size_of(T), align_of(T), allocator)
	assert(err == .None)
	if err != .None {
		return SlotMap(T){}
	}
	data_slot_index_raw, err_ := mem.alloc(cap * size_of(int), align_of(int), allocator)
	assert(err_ == .None)
	if err_ != .None {
		return SlotMap(T){}
	}
	return SlotMap(T) {
		data_raw = cast([^]T)data_raw,
		data_slot_index_raw = cast([^]int)data_slot_index_raw,
		len = 0,
		cap = cap,
		slots = make([dynamic]SlotMapKey, 0, cap, allocator),
		next_slot_index = 0,
		allocator = allocator,
	}
}
slotmap_new :: proc {
	slotmap_new_no_cap,
	slotmap_new_with_cap,
}

slotmap_handle_reserve_data :: proc(s: ^SlotMap($T), new_cap: int) {
	if new_cap <= s.cap do return

	new_data_raw, err := mem.alloc(new_cap * size_of(T), align_of(T), s.allocator)
	assert(err == .None)
	if err != .None {
		return
	}
	new_data_slot_index_raw, err_ := mem.alloc(new_cap * size_of(int), align_of(int), s.allocator)
	assert(err_ == .None)
	if err_ != .None {
		mem.free(new_data_raw, s.allocator)
		return
	}

	if s.data_raw != nil {
		mem.copy(new_data_raw, s.data_raw, s.len * size_of(T))
		free(s.data_raw, s.allocator)
	}
	if s.data_slot_index_raw != nil {
		mem.copy(new_data_slot_index_raw, s.data_slot_index_raw, s.len * size_of(int))
		free(s.data_slot_index_raw, s.allocator)
	}

	s.data_raw = cast([^]T)new_data_raw
	s.data_slot_index_raw = cast([^]int)new_data_slot_index_raw

	s.cap = new_cap
}

slotmap_handle_append_data :: proc(s: ^SlotMap($T), val: T, slot_index: int) {
	if (s.len + 1 > s.cap) {
		new_cap := max(4, s.cap + (s.cap >> 1))
		slotmap_handle_reserve_data(s, new_cap)
	}

	s.data_raw[s.len] = val
	s.data_slot_index_raw[s.len] = slot_index

	s.len += 1
}

slotmap_handle_pop_data :: proc(s: ^SlotMap($T)) {
	assert(s.len > 0)
	s.len -= 1
}

slotmap_handle_clear_data :: proc(s: ^SlotMap($T)) {
	s.len = 0
}

slotmap_append :: proc(s: ^SlotMap($T), val: T) -> SlotMapKey {
	slotmap_handle_append_data(s, val, s.next_slot_index)

	if s.next_slot_index == len(s.slots) {
		append(&s.slots, SlotMapKey{index = s.next_slot_index + 1, gen = 0})
	}

	slot_ptr := &s.slots[s.next_slot_index]
	index := s.next_slot_index

	s.next_slot_index = slot_ptr.index
	slot_ptr.index = s.len - 1

	return SlotMapKey{index = index, gen = slot_ptr.gen}
}

slotmap_remove :: proc(s: ^SlotMap($T), key: SlotMapKey) -> bool {
	if key.index >= len(s.slots) do return false

	slot_ptr := &s.slots[key.index]
	if slot_ptr.gen != key.gen do return false

	data_index := slot_ptr.index
	data_ptr := &s.data_raw[data_index]
	last_data_ptr := &s.data_raw[s.len - 1]

	if data_ptr != last_data_ptr {
		last_data_slot_index := s.data_slot_index_raw[s.len - 1]
		last_data_slot_ptr := &s.slots[last_data_slot_index]

		data_ptr^ = last_data_ptr^

		last_data_slot_ptr.index = data_index

		s.data_slot_index_raw[data_index] = last_data_slot_index
	}

	slotmap_handle_pop_data(s)

	slot_ptr.index = s.next_slot_index
	s.next_slot_index = key.index

	slot_ptr.gen += 1

	return true
}

slotmap_get :: proc(s: ^SlotMap($T), key: SlotMapKey) -> (T, bool) {
	index := key.index
	if index >= len(s.slots) do return {}, false

	slot := s.slots[index]
	if key.gen != slot.gen do return {}, false

	return s.data_raw[slot.index], true
}
slotmap_get_ptr :: proc(s: ^SlotMap($T), key: SlotMapKey) -> ^T {
	index := key.index
	if index >= len(s.slots) do return nil

	slot := s.slots[index]
	if key.gen != slot.gen do return nil

	return &s.data_raw[slot.index]
}

slotmap_clear :: proc(s: ^SlotMap($T)) {
	slotmap_handle_clear_data(s)

	for i in 0 ..< len(s.slots) {
		s.slots[i].gen += 1
		s.slots[i].index = i + 1
	}
	s.next_slot_index = 0
}

slotmap_reserve :: proc(s: ^SlotMap($T), new_cap: int) {
	slotmap_handle_reserve_data(s, new_cap)
	reserve(&s.slots, new_cap)
}

slotmap_len :: proc(s: ^SlotMap($T)) -> int {
	return s.len
}
slotmap_cap :: proc(s: ^SlotMap($T)) -> int {
	return s.cap
}

slotmap_iterate :: proc(s: ^SlotMap($T), i: ^int) -> (val: T, index: int, ok: bool) {
	if i^ >= s.len do return

	index = i^
	val = s.data_raw[index]
	ok = true

	i^ += 1

	return
}
slotmap_iterate_ptr :: proc(s: ^SlotMap($T), i: ^int) -> (val: ^T, index: int, ok: bool) {
	if i^ >= s.len do return

	index = i^
	val = &s.data_raw[index]
	ok = true

	i^ += 1

	return
}

slotmap_delete :: proc(s: ^SlotMap($T)) {
	mem.free(s.data_raw, s.allocator)
	mem.free(s.data_slot_index_raw, s.allocator)
	delete(s.slots)
}
