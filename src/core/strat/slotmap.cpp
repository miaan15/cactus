module;
export module cactus.core.strat:slotmap;

import std;

using size_t = std::size_t;

namespace cactus {

export struct SlotMapKey {
    size_t index;
    size_t gen;
};

export template <typename T, typename Alloc = std::allocator<T>> struct SlotMap {
    using AllocTraits = std::allocator_traits<Alloc>;
    using SlotAlloc = typename AllocTraits::template rebind_alloc<SlotMapKey>;
    using SlotVector = std::vector<SlotMapKey, SlotAlloc>;

    using iterator = T *;
    using const_iterator = const T *;

    T *data_raw;
    size_t *data_slot_index_raw;
    size_t len;
    size_t cap;

    SlotVector slots;
    size_t next_slot_index;

    [[nodiscard]] static auto make(size_t cap = 0) -> SlotMap {
        SlotVector slots{};

        T *data_raw = nullptr;
        size_t *data_slot_index_raw = nullptr;
        if (cap != 0) {
            auto slot_alloc = slots.get_allocator();
            using TAlloc = typename std::allocator_traits<SlotAlloc>::template rebind_alloc<T>;
            using SizeAlloc = typename std::allocator_traits<SlotAlloc>::template rebind_alloc<size_t>;

            TAlloc t_alloc(slot_alloc);
            SizeAlloc size_alloc(slot_alloc);

            data_raw = std::allocator_traits<TAlloc>::allocate(t_alloc, cap);
            data_slot_index_raw = std::allocator_traits<SizeAlloc>::allocate(size_alloc, cap);
        }

        return SlotMap{
            .data_raw = data_raw,
            .data_slot_index_raw = data_slot_index_raw,
            .len = 0,
            .cap = cap,
            .slots = std::move(slots),
            .next_slot_index = 0,
        };
    }

    auto destroy() const {
        auto slot_alloc = this->slots.get_allocator();

        if (this->data_raw != nullptr) {
            using TAlloc = typename std::allocator_traits<SlotAlloc>::template rebind_alloc<T>;
            TAlloc t_alloc(slot_alloc);
            std::allocator_traits<TAlloc>::deallocate(t_alloc, this->data_raw, this->cap);
        }

        if (this->data_slot_index_raw != nullptr) {
            using SizeAlloc = typename std::allocator_traits<SlotAlloc>::template rebind_alloc<size_t>;
            SizeAlloc size_alloc(slot_alloc);
            std::allocator_traits<SizeAlloc>::deallocate(size_alloc, this->data_slot_index_raw, this->cap);
        }
    }

    [[nodiscard]] auto push(const T &val) -> SlotMapKey {
        handle_append_data(val, this->next_slot_index);

        if (this->next_slot_index == this->slots.size()) {
            this->slots.push_back(SlotMapKey{.index = this->next_slot_index + 1, .gen = 0});
        }

        SlotMapKey *slot_ptr = &this->slots[this->next_slot_index];
        size_t index = this->next_slot_index;

        this->next_slot_index = slot_ptr->index;
        slot_ptr->index = this->len - 1;

        return SlotMapKey{.index = index, .gen = slot_ptr->gen};
    }

    auto remove(const SlotMapKey &key) -> bool {
        if (key.index >= this->slots.size()) return false;

        SlotMapKey *slot_ptr = &this->slots[key.index];
        if (slot_ptr->gen != key.gen) return false;

        size_t data_index = slot_ptr->index;
        T *data_ptr = &this->data_raw[data_index];
        T *last_data_ptr = &this->data_raw[this->len - 1];

        if (data_ptr != last_data_ptr) {
            size_t last_data_slot_index = this->data_slot_index_raw[this->len - 1];
            SlotMapKey *last_data_slot_ptr = &this->slots[last_data_slot_index];

            *data_ptr = *last_data_ptr;

            last_data_slot_ptr->index = data_index;

            this->data_slot_index_raw[data_index] = last_data_slot_index;
        }

        handle_pop_data();

        slot_ptr->index = this->next_slot_index;
        this->next_slot_index = key.index;

        ++slot_ptr->gen;

        return true;
    }

    [[nodiscard]] auto get(const SlotMapKey &key) const -> std::optional<const T *> {
        size_t index = key.index;
        if (index >= this->slots.size()) return {};

        SlotMapKey slot = this->slots[index];
        if (key.gen != slot.gen) return {};

        return &this->data_raw[slot.index];
    }
    [[nodiscard]] auto get_ptr(const SlotMapKey &key) -> std::optional<T *> {
        size_t index = key.index;
        if (index >= this->slots.size()) return {};

        SlotMapKey slot = this->slots[index];
        if (key.gen != slot.gen) return {};

        return &this->data_raw[slot.index];
    }

    [[nodiscard]] auto has(const SlotMapKey &key) const -> bool {
        size_t index = key.index;
        if (index >= this->slots.size()) return false;

        SlotMapKey slot = this->slots[index];
        if (key.gen != slot.gen) return false;

        return true;
    }

    auto clear() {
        handle_clear_data();

        for (size_t i = 0; i < this->slots.size(); i++) {
            ++this->slots[i].gen;
            this->slots[i].index = i + 1;
        }
        this->next_slot_index = 0;
    }

    auto reserve(size_t cap) {
        handle_reserve_data(cap);
        this->slots.reserve(cap);
    }

    [[nodiscard]] auto size() -> size_t { return this->len; }
    [[nodiscard]] auto capacity() -> size_t { return this->cap; }

    auto begin() -> iterator { return this->data_raw; }
    auto end() -> iterator { return this->data_raw + this->len; }
    auto begin() const -> const_iterator { return this->data_raw; }
    auto end() const -> const_iterator { return this->data_raw + this->len; }
    auto cbegin() const -> const_iterator { return this->data_raw; }
    auto cend() const -> const_iterator { return this->data_raw + this->len; }

private:
    auto handle_reserve_data(size_t cap) {
        if (cap <= this->cap) return;

        auto slot_alloc = this->slots.get_allocator();
        using TAlloc = typename std::allocator_traits<SlotAlloc>::template rebind_alloc<T>;
        using SizeAlloc = typename std::allocator_traits<SlotAlloc>::template rebind_alloc<size_t>;

        TAlloc t_alloc(slot_alloc);
        SizeAlloc size_alloc(slot_alloc);

        T *new_data_raw = std::allocator_traits<TAlloc>::allocate(t_alloc, cap);
        size_t *new_data_slot_index_raw = std::allocator_traits<SizeAlloc>::allocate(size_alloc, cap);

        if (this->data_raw != nullptr) {
            std::memcpy(new_data_raw, this->data_raw, this->len * sizeof(T));
            std::allocator_traits<TAlloc>::deallocate(t_alloc, this->data_raw, this->cap);
        }
        if (this->data_slot_index_raw != nullptr) {
            std::memcpy(new_data_slot_index_raw, this->data_slot_index_raw, this->len * sizeof(size_t));
            std::allocator_traits<SizeAlloc>::deallocate(size_alloc, this->data_slot_index_raw, this->cap);
        }

        this->data_raw = new_data_raw;
        this->data_slot_index_raw = new_data_slot_index_raw;
        this->cap = cap;
    }

    auto handle_append_data(const T &val, size_t slot_index) {
        if (this->len + 1 > this->cap) {
            size_t new_cap = this->cap * 2;
            if (new_cap < 4) new_cap = 4;
            handle_reserve_data(new_cap);
        }

        this->data_raw[this->len] = val;
        this->data_slot_index_raw[this->len] = slot_index;

        ++this->len;
    }

    auto handle_pop_data() { --this->len; }
    auto handle_clear_data() { this->len = 0; }
};

} // namespace cactus
