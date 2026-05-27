module;

#include <climits>

export module cactus.core.strat:slot_map;

import std;
import :dynamic_array;

using size_t = std::size_t;

namespace cactus {

export struct SlotMapKey {
    size_t index : (sizeof(size_t) * CHAR_BIT) - CHAR_BIT;
    size_t gen : CHAR_BIT;

    operator size_t() const { return *reinterpret_cast<const size_t *>(this); }
};

export template <typename T, typename Alloc = std::allocator<T>>
    requires std::is_trivially_copyable_v<T>
struct SlotMap {
    using allocator_traits_t = std::allocator_traits<Alloc>;
    using slot_allocator_t = typename allocator_traits_t::template rebind_alloc<SlotMapKey>;
    using slot_container_t = DynamicArray<SlotMapKey, slot_allocator_t>;

    T *data = nullptr;
    size_t *slot_indexes = nullptr;
    size_t len = 0;
    size_t cap = 0;

    slot_container_t slots = slot_container_t::make();
    size_t next_slot_index = 0;

    [[nodiscard]] static auto make() noexcept -> SlotMap { return SlotMap{}; }
    auto destroy() noexcept {
        auto slot_alloc = slots.allocator;

        if (data) {
            using T_alloc_t = typename std::allocator_traits<slot_allocator_t>::template rebind_alloc<T>;
            using T_alloc_traits_t = std::allocator_traits<T_alloc_t>;
            T_alloc_t t_alloc{slot_alloc};
            T_alloc_traits_t::deallocate(t_alloc, data, cap);
        }

        if (slot_indexes) {
            using size_alloc_t = typename std::allocator_traits<slot_allocator_t>::template rebind_alloc<T>;
            using size_alloc_traits_t = std::allocator_traits<size_alloc_t>;
            size_alloc_t size_alloc{slot_alloc};
            size_alloc_traits_t::deallocate(size_alloc, data, cap);
        }
    }
    [[nodiscard]] auto clone() noexcept -> SlotMap {
        if (cap == 0) return SlotMap::make();

        auto slot_alloc = slots.allocator;

        using T_alloc_t = typename std::allocator_traits<slot_allocator_t>::template rebind_alloc<T>;
        using T_alloc_traits_t = std::allocator_traits<T_alloc_t>;
        T_alloc_t t_alloc{slot_alloc};
        T *new_data = T_alloc_traits_t::allocate(t_alloc, cap);
        std::memcpy(new_data, data, cap * sizeof(T));

        using size_alloc_t = typename std::allocator_traits<slot_allocator_t>::template rebind_alloc<T>;
        using size_alloc_traits_t = std::allocator_traits<size_alloc_t>;
        size_alloc_t size_alloc{slot_alloc};
        size_t *new_slot_indexes = size_alloc_traits_t::allocate(size_alloc, cap);
        std::memcpy(new_slot_indexes, slot_indexes, cap * sizeof(size_t));

        return SlotMap{.data = new_data,
                       .slot_indexes = new_slot_indexes,
                       .len = len,
                       .cap = cap,
                       .slots = slots.clone(),
                       .next_slot_index = next_slot_index};
    }

    [[nodiscard]] auto add(const T &val) noexcept -> SlotMapKey {
        handle_append_data(val, next_slot_index);

        if (next_slot_index == slots.len) slots.append(SlotMapKey{.index = next_slot_index + 1, .gen = 0});

        SlotMapKey *slot_ptr = slots.get_ptr(next_slot_index);
        size_t index = next_slot_index;

        next_slot_index = slot_ptr->index;
        slot_ptr->index = len - 1;

        return SlotMapKey{.index = index, .gen = slot_ptr->gen};
    }

    auto remove(SlotMapKey key) noexcept -> bool {
        if (key.index >= slots.len) return false;

        SlotMapKey *slot_ptr = slots.get_ptr(key.index);
        if (slot_ptr->gen != key.gen) return false;

        size_t data_index = slot_ptr->index;
        T *data_ptr = &data[data_index];
        T *last_data_ptr = &data[len - 1];

        if (data_ptr != last_data_ptr) {
            size_t last_slot_index = slot_indexes[len - 1];
            SlotMapKey *last_slot_ptr = slots.get_ptr(last_slot_index);

            *data_ptr = *last_data_ptr;

            last_slot_ptr->index = data_index;

            slot_indexes[data_index] = last_slot_index;
        }

        handle_pop_data();

        slot_ptr->index = next_slot_index;
        next_slot_index = key.index;

        ++slot_ptr->gen;

        return true;
    }

    auto set(SlotMapKey key, const T &val) noexcept -> bool {
        size_t index = key.index;
        if (index >= slots.len) return false;

        SlotMapKey slot = slots.get(index).value();
        if (key.gen != slot.gen) return false;

        data[slot.index] = val;
        return true;
    }

    [[nodiscard]] auto get(SlotMapKey key) const noexcept -> std::optional<T> {
        size_t index = key.index;
        if (index >= slots.len) return {};

        SlotMapKey slot = slots.get(index).value();
        if (key.gen != slot.gen) return {};

        return data[slot.index];
    }

    [[nodiscard]] auto get_ptr(SlotMapKey key) noexcept -> T * {
        size_t index = key.index;
        if (index >= slots.len) return nullptr;

        SlotMapKey slot = slots.get(index).value();
        if (key.gen != slot.gen) return nullptr;

        return &data[slot.index];
    }
    [[nodiscard]] auto get_ptr(SlotMapKey key) const noexcept -> const T * {
        size_t index = key.index;
        if (index >= slots.len) return nullptr;

        SlotMapKey slot = slots.get(index).value();
        if (key.gen != slot.gen) return nullptr;

        return &data[slot.index];
    }

    [[nodiscard]] auto has(SlotMapKey key) const noexcept -> bool {
        size_t index = key.index;
        if (index >= slots.len) return false;

        SlotMapKey slot = slots.get(index).value();
        if (key.gen != slot.gen) return false;

        return true;
    }

    auto clear() noexcept {
        handle_clear_data();

        for (size_t i = 0; i < slots.len; i++) {
            SlotMapKey *slot_ptr = slots.get_ptr(i);
            ++slot_ptr->gen;
            slot_ptr->index = i + 1;
        }
        next_slot_index = 0;
    }

    auto reserve(size_t cap) noexcept {
        handle_reserve_data(cap);
        slots.reserve(cap);
    }

    [[nodiscard]] auto empty() const noexcept { return len == 0; }

    using iterator = T *;
    using const_iterator = const T *;

    auto begin() -> iterator { return data; }
    auto end() -> iterator { return data + len; }
    auto begin() const -> const_iterator { return data; }
    auto end() const -> const_iterator { return data + len; }
    auto cbegin() const -> const_iterator { return data; }
    auto cend() const -> const_iterator { return data + len; }

private:
    auto handle_reserve_data(size_t new_cap) noexcept {
        if (new_cap <= cap) return;

        auto slot_alloc = slots.allocator;
        using t_allloc_t = typename std::allocator_traits<slot_allocator_t>::template rebind_alloc<T>;
        using size_alloc_t = typename std::allocator_traits<slot_allocator_t>::template rebind_alloc<size_t>;

        using t_alloc_traits_t = std::allocator_traits<t_allloc_t>;
        t_allloc_t t_alloc(slot_alloc);

        using size_alloc_traits_t = std::allocator_traits<size_alloc_t>;
        size_alloc_t size_alloc(slot_alloc);

        T *new_data = t_alloc_traits_t::allocate(t_alloc, new_cap);
        size_t *new_slot_indexes = size_alloc_traits_t::allocate(size_alloc, new_cap);

        if (data != nullptr) {
            std::memcpy(new_data, data, len * sizeof(T));
            t_alloc_traits_t::deallocate(t_alloc, data, cap);
        }
        if (slot_indexes != nullptr) {
            std::memcpy(new_slot_indexes, slot_indexes, len * sizeof(size_t));
            size_alloc_traits_t::deallocate(size_alloc, slot_indexes, cap);
        }

        data = new_data;
        slot_indexes = new_slot_indexes;
        cap = new_cap;
    }

    auto handle_append_data(const T &val, size_t slot_index) noexcept {
        if (len + 1 > cap) {
            size_t new_cap = cap == 0 ? 4 : cap + (cap / 2);
            handle_reserve_data(new_cap);
        }

        data[len] = val;
        slot_indexes[len] = slot_index;
        ++len;
    }

    auto handle_pop_data() noexcept { --len; }
    auto handle_clear_data() noexcept { len = 0; }
};

} // namespace cactus
