module;

export module cactus.core.strat:hash_map;

import std;
import :assert;

using size_t = std::size_t;

namespace cactus {

export template <typename K, typename V, typename Hash = std::hash<K>, typename KeyEqual = std::equal_to<K>,
                 typename Alloc = std::allocator<std::pair<K, V>>>
    requires std::is_trivially_copyable_v<V>
struct HashMap {
    enum SlotState : char { EMPTY = 0, OCCUPIED, DELETED };

    using alloc_traits_t = std::allocator_traits<Alloc>;
    using state_alloc_t = alloc_traits_t::template rebind_alloc<SlotState>;
    using state_alloc_traits_t = std::allocator_traits<state_alloc_t>;
    using slot_t = std::pair<K, V>;

    slot_t *slots = nullptr;
    SlotState *states = nullptr;
    size_t len = 0;
    size_t cap = 0;
    size_t deleted_count = 0;

    [[no_unique_address]] Alloc allocator = Alloc();
    [[no_unique_address]] state_alloc_t state_allocator = state_alloc_t();

    [[nodiscard]] static auto make() noexcept -> HashMap { return HashMap{}; }
    auto destroy() noexcept {
        if (slots) {
            for (size_t i = 0; i < cap; ++i) {
                if (states[i] == OCCUPIED) slots[i].~slot_t();
            }
            alloc_traits_t::deallocate(allocator, slots, cap);
            state_alloc_traits_t::deallocate(state_allocator, states, cap);
            slots = nullptr;
            states = nullptr;
        }
        len = 0;
        cap = 0;
        deleted_count = 0;
    }
    [[nodiscard]] auto clone() const noexcept -> HashMap {
        if (cap < 8) return HashMap::make();

        Alloc new_allocator = Alloc(allocator);
        state_alloc_t new_state_allocator = state_alloc_t(state_allocator);
        slot_t *new_slots = alloc_traits_t::allocate(new_allocator, cap);
        SlotState *new_states = state_alloc_traits_t::allocate(new_state_allocator, cap);

        std::memcpy(new_states, states, cap * sizeof(SlotState));
        for (size_t i = 0; i < cap; ++i) {
            if (states[i] == OCCUPIED) new (&new_slots[i]) slot_t(slots[i]);
        }

        return HashMap{.slots = new_slots,
                       .states = new_states,
                       .len = len,
                       .cap = cap,
                       .deleted_count = deleted_count,
                       .allocator = new_allocator,
                       .state_allocator = new_state_allocator};
    }

    auto rehash(size_t new_cap) noexcept {
        _assert((new_cap & (new_cap - 1)) == 0, "HashMap rehash(): new_cap must be a power of two");

        slot_t *new_slots = alloc_traits_t::allocate(allocator, new_cap);
        if (!new_slots) _assert(false, "HashMap failed to allocate memory");
        SlotState *new_states = state_alloc_traits_t::allocate(state_allocator, new_cap);
        if (!new_states) _assert(false, "HashMap failed to allocate memory");
        std::memset(new_states, EMPTY, new_cap * sizeof(SlotState));

        if (slots) {
            for (size_t i = 0; i < cap; ++i) {
                if (states[i] == OCCUPIED) {
                    size_t hash = Hash{}(slots[i].first);
                    size_t index = hash & (new_cap - 1);

                    while (new_states[index] != EMPTY) index = (index + 1) & (new_cap - 1);

                    new (&new_slots[index]) slot_t(std::move(slots[i]));
                    new_states[index] = OCCUPIED;

                    slots[i].~slot_t();
                }
            }
            alloc_traits_t::deallocate(allocator, slots, cap);
            state_alloc_traits_t::deallocate(state_allocator, states, cap);
        }

        slots = new_slots;
        states = new_states;
        cap = new_cap;
        deleted_count = 0;
    }

    auto add(const K &key, const V &value) noexcept -> bool {
        if ((len + deleted_count) * 10 >= cap * 7) { rehash(cap < 8 ? 8 : cap * 2); }

        constexpr size_t EMPTY_DELETED_I = static_cast<size_t>(-1);
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        size_t first_deleted = EMPTY_DELETED_I;

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) { return false; }
            if (states[index] == DELETED && first_deleted == EMPTY_DELETED_I) { first_deleted = index; }
            index = (index + 1) & (cap - 1);
        }

        size_t target = first_deleted != EMPTY_DELETED_I ? first_deleted : index;

        new (&slots[target]) slot_t(key, value);
        states[target] = OCCUPIED;
        ++len;

        if (target == first_deleted) --deleted_count;

        return true;
    }
    auto add(K &&key, const V &value) noexcept -> bool {
        if ((len + deleted_count) * 10 >= cap * 7) { rehash(cap < 8 ? 8 : cap * 2); }

        constexpr size_t EMPTY_DELETED_I = static_cast<size_t>(-1);
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        size_t first_deleted = EMPTY_DELETED_I;

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) { return false; }
            if (states[index] == DELETED && first_deleted == EMPTY_DELETED_I) { first_deleted = index; }
            index = (index + 1) & (cap - 1);
        }

        size_t target = first_deleted != EMPTY_DELETED_I ? first_deleted : index;

        new (&slots[target]) slot_t(std::move(key), value);
        states[target] = OCCUPIED;
        ++len;

        if (target == first_deleted) --deleted_count;

        return true;
    }

    auto set(const K &key, const V &value) noexcept -> bool {
        if ((len + deleted_count) * 10 >= cap * 7) { rehash(cap < 8 ? 8 : cap * 2); }

        constexpr size_t EMPTY_DELETED_I = static_cast<size_t>(-1);
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        size_t first_deleted = EMPTY_DELETED_I;

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) {
                slots[index].second = value;
                return true;
            }
        }

        return false;
    }
    auto set(K &&key, const V &value) noexcept -> bool {
        if ((len + deleted_count) * 10 >= cap * 7) { rehash(cap < 8 ? 8 : cap * 2); }

        constexpr size_t EMPTY_DELETED_I = static_cast<size_t>(-1);
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        size_t first_deleted = EMPTY_DELETED_I;

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) {
                slots[index].second = value;
                return true;
            }
        }

        return false;
    }

    auto add_or_set(const K &key, const V &value) noexcept {
        if ((len + deleted_count) * 10 >= cap * 7) { rehash(cap < 8 ? 8 : cap * 2); }

        constexpr size_t EMPTY_DELETED_I = static_cast<size_t>(-1);
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        size_t first_deleted = EMPTY_DELETED_I;

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) {
                slots[index].second = value;
                return;
            }
            if (states[index] == DELETED && first_deleted == EMPTY_DELETED_I) { first_deleted = index; }
            index = (index + 1) & (cap - 1);
        }

        size_t target = first_deleted != EMPTY_DELETED_I ? first_deleted : index;

        new (&slots[target]) slot_t(key, value);
        states[target] = OCCUPIED;
        ++len;

        if (target == first_deleted) --deleted_count;
    }
    auto add_or_set(K &&key, const V &value) noexcept {
        if ((len + deleted_count) * 10 >= cap * 7) { rehash(cap < 8 ? 8 : cap * 2); }

        constexpr size_t EMPTY_DELETED_I = static_cast<size_t>(-1);
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        size_t first_deleted = EMPTY_DELETED_I;

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) {
                slots[index].second = value;
                return;
            }
            if (states[index] == DELETED && first_deleted == EMPTY_DELETED_I) { first_deleted = index; }
            index = (index + 1) & (cap - 1);
        }

        size_t target = first_deleted != EMPTY_DELETED_I ? first_deleted : index;

        new (&slots[target]) slot_t(std::move(key), value);
        states[target] = OCCUPIED;
        ++len;

        if (target == first_deleted) --deleted_count;
    }

    auto remove(const K &key) noexcept -> bool {
        if (cap < 8) return false;

        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) {
                slots[index].~slot_t();
                states[index] = DELETED;

                --len;
                ++deleted_count;

                return true;
            }
            index = (index + 1) & (cap - 1);
        }
        return false;
    }

    [[nodiscard]] auto get(const K &key) const noexcept -> std::optional<V> {
        if (cap < 8) return {};
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) { return slots[index].second; }
            index = (index + 1) & (cap - 1);
        }
        return {};
    }

    [[nodiscard]] auto get_ptr(const K &key) noexcept -> V * {
        if (cap < 8) return nullptr;
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) { return &slots[index].second; }
            index = (index + 1) & (cap - 1);
        }
        return nullptr;
    }
    [[nodiscard]] auto get_ptr(const K &key) const noexcept -> const V * {
        if (cap < 8) return nullptr;
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) { return &slots[index].second; }
            index = (index + 1) & (cap - 1);
        }
        return nullptr;
    }

    [[nodiscard]] auto get_or_add_ptr(const K &key) noexcept -> V * {
        if ((len + deleted_count) * 10 >= cap * 7) { rehash(cap < 8 ? 8 : cap * 2); }

        constexpr size_t EMPTY_DELETED_I = static_cast<size_t>(-1);
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        size_t first_deleted = EMPTY_DELETED_I;

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) { return &slots[index].second; }
            if (states[index] == DELETED && first_deleted == EMPTY_DELETED_I) { first_deleted = index; }
            index = (index + 1) & (cap - 1);
        }

        size_t target = first_deleted != EMPTY_DELETED_I ? first_deleted : index;

        new (&slots[target]) slot_t(key, V{});
        states[target] = OCCUPIED;
        ++len;

        if (target == first_deleted) --deleted_count;

        return &slots[target].second;
    }
    [[nodiscard]] auto get_or_add_ptr(K &&key) noexcept -> V * {
        if ((len + deleted_count) * 10 >= cap * 7) { rehash(cap < 8 ? 8 : cap * 2); }

        constexpr size_t EMPTY_DELETED_I = static_cast<size_t>(-1);
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        size_t first_deleted = EMPTY_DELETED_I;

        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) { return &slots[index].second; }
            if (states[index] == DELETED && first_deleted == EMPTY_DELETED_I) { first_deleted = index; }
            index = (index + 1) & (cap - 1);
        }

        size_t target = first_deleted != EMPTY_DELETED_I ? first_deleted : index;

        new (&slots[target]) slot_t(std::move(key), V{});
        states[target] = OCCUPIED;
        ++len;

        if (target == first_deleted) --deleted_count;

        return &slots[target].second;
    }

    [[nodiscard]] auto has(const K &key) const noexcept -> bool {
        if (cap < 8) return false;
        size_t hash = Hash{}(key);
        size_t index = hash & (cap - 1);
        while (states[index] != EMPTY) {
            if (states[index] == OCCUPIED && KeyEqual{}(slots[index].first, key)) { return true; }
            index = (index + 1) & (cap - 1);
        }
        return false;
    }

    auto clear() noexcept {
        if (!slots) return;

        for (size_t i = 0; i < cap; ++i) {
            if (states[i] == OCCUPIED) slots[i].~slot_t();
        }

        std::memset(states, EMPTY, cap * sizeof(SlotState));
        len = 0;
        deleted_count = 0;
    }

    [[nodiscard]] auto empty() const noexcept -> bool { return len == 0; }

    template <bool IsConst> struct IteratorImpl {
        using map_type = std::conditional_t<IsConst, const HashMap, HashMap>;
        using map_ptr = map_type *;
        using slot_type = std::conditional_t<IsConst, const slot_t, slot_t>;

        using iterator_category = std::forward_iterator_tag;
        using value_type = slot_t;
        using difference_type = std::ptrdiff_t;
        using pointer = slot_type *;
        using reference = slot_type &;

        map_ptr map;
        size_t index;

        IteratorImpl(map_ptr m, size_t i) noexcept : map(m), index(i) { advance_to_valid(); }

        auto advance_to_valid() noexcept -> void {
            while (index < map->cap && map->states[index] != OCCUPIED) { ++index; }
        }

        [[nodiscard]] auto operator*() const noexcept -> reference { return map->slots[index]; }

        [[nodiscard]] auto operator->() const noexcept -> pointer { return &map->slots[index]; }

        auto operator++() noexcept -> IteratorImpl & {
            ++index;
            advance_to_valid();
            return *this;
        }

        auto operator++(int) noexcept -> IteratorImpl {
            IteratorImpl tmp = *this;
            ++(*this);
            return tmp;
        }

        [[nodiscard]] friend auto operator==(const IteratorImpl &a, const IteratorImpl &b) noexcept -> bool {
            return a.index == b.index;
        }

        [[nodiscard]] friend auto operator!=(const IteratorImpl &a, const IteratorImpl &b) noexcept -> bool {
            return a.index != b.index;
        }
    };

    using iterator = IteratorImpl<false>;
    using const_iterator = IteratorImpl<true>;

    [[nodiscard]] auto begin() noexcept -> iterator { return iterator(this, 0); }
    [[nodiscard]] auto end() noexcept -> iterator { return iterator(this, cap); }

    [[nodiscard]] auto begin() const noexcept -> const_iterator { return const_iterator(this, 0); }
    [[nodiscard]] auto end() const noexcept -> const_iterator { return const_iterator(this, cap); }

    [[nodiscard]] auto cbegin() const noexcept -> const_iterator { return begin(); }
    [[nodiscard]] auto cend() const noexcept -> const_iterator { return end(); }
};

} // namespace cactus
