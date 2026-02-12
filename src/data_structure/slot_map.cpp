// Implement follow
// https://github.com/WG21-SG14/SG14/blob/master/SG14/slot_map.h

module;

#include <boost/container/vector.hpp>
#include <cstdint>
#include <optional>
#include <utility>

export module SlotMap;

namespace cactus {

export [[nodiscard]] constexpr auto get_idx(uint64_t k) -> uint32_t {
    return static_cast<uint32_t>(k >> 32);
}
export [[nodiscard]] constexpr auto get_gen(uint64_t k) -> uint32_t {
    return static_cast<uint32_t>(k);
}
export constexpr auto set_idx(uint64_t *k, uint32_t value) -> void {
    *k = (*k & 0xFFFFFFFFULL) | (static_cast<uint64_t>(value) << 32);
}
export constexpr auto increase_gen(uint64_t *k) -> void {
    uint32_t gen = static_cast<uint32_t>(*k) + 1;
    *k = (*k & ~0xFFFFFFFFULL) | static_cast<uint64_t>(gen);
}

export template <typename T, template <class...> class Container = boost::container::vector>
    requires std::is_same<typename Container<T>::value_type, T>::value
struct SlotMap {
    using value_type = T;
    using key_type = uint64_t;
    using key_index_type = uint32_t;
    using key_gen_type = uint32_t;
    using container_type = Container<T>;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;
    using size_type = typename container_type::size_type;

    Container<key_type> slots;
    Container<key_index_type> data_map;
    Container<value_type> data;
    key_index_type next_slot_idx = 0;

    using slot_iterator = decltype(slots)::iterator;

    constexpr auto find(key_type key) -> iterator {
        auto slot_idx = get_idx(key);
        if (slot_idx >= slots.size()) return end();

        auto slot_iter = std::next(slots.begin(), slot_idx);
        if (get_gen(*slot_iter) != get_gen(key)) return end();

        return std::next(data.begin(), get_idx(*slot_iter));
    }
    constexpr auto find(key_type key) const -> const_iterator {
        auto slot_idx = get_idx(key);
        if (slot_idx >= slots.size()) return end();

        auto slot_iter = std::next(slots.begin(), slot_idx);
        if (get_gen(*slot_iter) != get_gen(key)) return end();

        return std::next(data.begin(), get_idx(*slot_iter));
    }

    constexpr auto at(key_type key) -> std::optional<pointer> {
        auto data_iter = this->find(key);
        if (data_iter == this->end()) return {};
        return &*data_iter;
    }
    constexpr auto at(key_type key) const -> std::optional<const_pointer> {
        auto data_iter = this->find(key);
        if (data_iter == this->end()) return {};
        return &*data_iter;
    }

    constexpr auto operator[](key_type key) -> reference {
        auto slot_iter = std::next(slots.begin(), get_idx(key));
        auto data_iter = std::next(data.begin(), get_idx(*slot_iter));
        return *data_iter;
    }
    constexpr auto operator[](key_type key) const -> const_reference {
        auto slot_iter = std::next(slots.begin(), get_idx(key));
        auto data_iter = std::next(data.begin(), get_idx(*slot_iter));
        return *data_iter;
    }

    template <class... Args>
    constexpr auto emplace(Args &&...args) {
        auto data_pos = data.size();
        data.emplace_back(std::forward<Args>(args)...);
        data_map.emplace_back(next_slot_idx);

        if (next_slot_idx == slots.size()) {
            slots.emplace_back(static_cast<key_type>(next_slot_idx + 1) << 32);
            // temporary set new slot idx = next_slot_idx + 1 for later update of next_slot_idx
            // (equal next_slot_idx + 1 if this branch true)
        }

        auto slot_iter = std::next(slots.begin(), next_slot_idx);
        next_slot_idx = get_idx(*slot_iter);
        set_idx(&*slot_iter, data_pos);

        key_type res = *slot_iter;
        set_idx(&res, std::distance(slots.begin(), slot_iter));
        return res;
    }

    constexpr auto insert(const value_type &value) -> key_type {
        return this->emplace(value);
    }
    constexpr auto insert(value_type &&value) -> key_type {
        return this->emplace(std::move(value));
    }

    constexpr auto erase(const_iterator pos) -> iterator {
        auto slot_iter = slot_iter_from_data_iter(pos);
        auto data_idx = get_idx(*slot_iter);
        auto data_iter = std::next(data.begin(), data_idx);
        auto last_data_iter = std::prev(data.end());

        if (data_iter != last_data_iter) {
            auto last_data_slot_iter = slot_iter_from_data_iter(last_data_iter);
            *data_iter = std::move(*last_data_iter);
            set_idx(&*last_data_slot_iter, data_idx);
            *std::next(data_map.begin(), data_idx) =
                std::distance(slots.begin(), last_data_slot_iter);
        }

        data.pop_back();
        data_map.pop_back();

        set_idx(&*slot_iter, next_slot_idx);
        next_slot_idx = std::distance(slots.begin(), slot_iter);

        increase_gen(&*slot_iter);
        return std::next(data.begin(), data_idx);
    }

    constexpr auto erase(const_iterator first, const_iterator last) -> iterator {
        auto first_index = std::distance(this->cbegin(), first);
        auto last_index = std::distance(this->cbegin(), last);
        while (last_index != first_index) {
            --last_index;
            auto iter = std::next(this->cbegin(), last_index);
            this->erase(iter);
        }
        return std::next(this->begin(), first_index);
    }

    constexpr auto erase(iterator pos) -> iterator {
        return this->erase(const_iterator(pos));
    }
    constexpr auto erase(iterator first, iterator last) -> iterator {
        return this->erase(const_iterator(first), const_iterator(last));
    }
    constexpr auto erase(key_type key) -> iterator {
        auto iter = this->find(key);
        if (iter == this->end()) {
            return end();
        }
        return this->erase(iter);
    }

    constexpr auto reserve(size_type n) -> void {
        data.reserve(n);
        data_map.reserve(n);
        slots.reserve(n);
    }

    constexpr auto clear() -> void {
        slots.clear();
        data.clear();
        data_map.clear();
        next_slot_idx = 0;
    }

    constexpr auto swap(SlotMap &rhs) -> void {
        using std::swap;
        swap(slots, rhs.slots);
        swap(data, rhs.data);
        swap(data_map, rhs.data_map);
        swap(next_slot_idx, rhs.next_slot_idx);
    }

    constexpr iterator begin() {
        return data.begin();
    }
    constexpr iterator end() {
        return data.end();
    }
    constexpr const_iterator begin() const {
        return data.begin();
    }
    constexpr const_iterator end() const {
        return data.end();
    }
    constexpr const_iterator cbegin() const {
        return data.begin();
    }
    constexpr const_iterator cend() const {
        return data.end();
    }
    constexpr reverse_iterator rbegin() {
        return data.rbegin();
    }
    constexpr reverse_iterator rend() {
        return data.rend();
    }
    constexpr const_reverse_iterator rbegin() const {
        return data.rbegin();
    }
    constexpr const_reverse_iterator rend() const {
        return data.rend();
    }
    constexpr const_reverse_iterator crbegin() const {
        return data.rbegin();
    }
    constexpr const_reverse_iterator crend() const {
        return data.rend();
    }

    constexpr auto empty() const -> bool {
        return data.size() == 0;
    }
    constexpr auto size() const -> size_type {
        return data.size();
    }
    constexpr auto capacity() const -> size_type {
        return data.capacity();
    }

    constexpr auto slot_iter_from_data_iter(const_iterator data_iter) -> slot_iterator {
        auto data_index = std::distance(const_iterator(data.begin()), data_iter);
        auto slot_index = *std::next(data_map.begin(), data_index);
        return std::next(slots.begin(), slot_index);
    }
};

export template <class T, template <class...> class Container>
constexpr void swap(SlotMap<T, Container> &lhs, SlotMap<T, Container> &rhs) {
    lhs.swap(rhs);
}

} // namespace cactus
