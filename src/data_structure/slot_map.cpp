// Implement follow
// https://github.com/WG21-SG14/SG14/blob/master/SG14/slot_map.h

module;

#include <boost/container/vector.hpp>
#include <cstdint>
#include <optional>

export module SlotMap;

namespace cactus {

[[nodiscard]] constexpr auto get_idx(uint64_t k) -> uint32_t {
    return static_cast<uint32_t>(k);
}
[[nodiscard]] constexpr auto get_gen(uint64_t k) -> uint32_t {
    return static_cast<uint32_t>(k >> 32);
}
constexpr auto set_idx(uint64_t *k, uint32_t value) -> void {
    *k = (*k & ~0xFFFFFFFFULL) | value;
}
constexpr auto increase_gen(uint64_t *k) -> void {
    k += (1ULL << 32);
}

export template <typename T, template <class...> class Container = boost::container::vector>
struct SlotMap {
    using KeyT = uint64_t;
    using KeyIdxT = uint32_t;
    using KeyGenT = uint32_t;
    using ContainerT = Container<T>;
    using Reference = typename ContainerT::reference;
    using ConstReference = typename ContainerT::const_reference;
    using Pointer = typename ContainerT::pointer;
    using ConstPointer = typename ContainerT::const_pointer;
    using Iterator = typename ContainerT::iterator;
    using ConstIterator = typename ContainerT::const_iterator;
    using ReverseIterator = typename ContainerT::reverse_iterator;
    using ConstReverseIterator = typename ContainerT::const_reverse_iterator;
    using SizeT = typename ContainerT::size_type;
    using ValueT = typename ContainerT::data_type;

    static_assert(std::is_same<ValueT, T>::value, "Container<T>::data_type must be identical to T");

    Container<KeyT> slots;
    Container<KeyIdxT> data_map;
    Container<ValueT> data;
    KeyIdxT next_slot_idx;

    using SlotIterator = decltype(slots);

    constexpr auto find(KeyT key) -> Iterator {
        auto sparse_idx = get_idx(key);
        if (sparse_idx >= slots.size()) return end();

        auto sparse_iter = std::next(slots.begin(), sparse_idx);
        if (get_gen(*sparse_iter) != get_gen(key)) return end();

        return std::next(data.begin(), get_idx(*sparse_iter));
    }
    constexpr auto find(KeyT key) const -> ConstIterator {
        auto sparse_idx = get_idx(key);
        if (sparse_idx >= slots.size()) return end();

        auto sparse_iter = std::next(slots.begin(), sparse_idx);
        if (get_gen(*sparse_iter) != get_gen(key)) return end();

        return std::next(data.begin(), get_idx(*sparse_iter));
    }

    constexpr auto at(KeyT key) -> std::optional<Reference> {
        auto data_iter = this->find(key);
        if (data_iter == this->end()) return {};
        return *data_iter;
    }
    constexpr auto at(KeyT key) const -> std::optional<ConstReference> {
        auto data_iter = this->find(key);
        if (data_iter == this->end()) return {};
        return *data_iter;
    }

    constexpr auto operator[](KeyT key) -> Reference {
        auto sparse_iter = std::next(slots.begin(), get_idx(key));
        auto data_iter = std::next(data.begin(), get_index(*sparse_iter));
        return data_iter;
    }
    constexpr auto operator[](KeyT key) const -> ConstReference {
        auto sparse_iter = std::next(slots.begin(), get_idx(key));
        auto data_iter = std::next(data.begin(), get_index(*sparse_iter));
        return data_iter;
    }

    template <class... Args>
    constexpr auto emplace(Args &&...args) {
        auto data_pos = data.size();
        data.emplace_back(std::forward(args)...);
        data_map.emplace_back(next_slot_idx);

        if (next_slot_idx == slots.size()) {
            slots.emplace_back(++next_slot_idx); // temporary set new slot idx = next_slot_idx + 1
                                                 // for later update of next_slot_idx (equal
                                                 // next_slot_idx + 1 if this branch true)
        }

        auto slot_iter = std::next(slots.begin(), next_slot_idx);
        next_slot_idx = get_idx(*slot_iter);
        set_idx(&*slot_iter, data_pos);
        return (*slot_iter & ~0xFFFFFFFFULL)
               | static_cast<KeyIdxT>(std::distance(slots.begin(), slot_iter));
    }

    constexpr auto insert(const ValueT &value) -> KeyT {
        return this->emplace(value);
    }
    constexpr auto insert(ValueT &&value) -> KeyT {
        return this->emplace(std::move(value));
    }

    constexpr auto erase(ConstIterator pos) -> Iterator {
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

    constexpr auto erase(ConstIterator first, ConstIterator last) -> Iterator {
        auto first_index = std::distance(this->cbegin(), first);
        auto last_index = std::distance(this->cbegin(), last);
        while (last_index != first_index) {
            --last_index;
            auto iter = std::next(this->cbegin(), last_index);
            this->erase(iter);
        }
        return std::next(this->begin(), first_index);
    }

    constexpr auto erase(Iterator pos) -> Iterator {
        return this->erase(const_iterator(pos));
    }
    constexpr auto erase(Iterator first, Iterator last) -> Iterator {
        return this->erase(const_iterator(first), const_iterator(last));
    }
    constexpr auto erase(KeyT key) -> SizeT {
        auto iter = this->find(key);
        if (iter == this->end()) {
            return 0;
        }
        this->erase(iter);
        return 1;
    }

    constexpr auto reserve(SizeT n) -> void {
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

    constexpr Iterator begin() {
        return data.begin();
    }
    constexpr Iterator end() {
        return data.end();
    }
    constexpr ConstIterator begin() const {
        return data.begin();
    }
    constexpr ConstIterator end() const {
        return data.end();
    }
    constexpr ConstIterator cbegin() const {
        return data.begin();
    }
    constexpr ConstIterator cend() const {
        return data.end();
    }
    constexpr ReverseIterator rbegin() {
        return data.rbegin();
    }
    constexpr ReverseIterator rend() {
        return data.rend();
    }
    constexpr ConstReverseIterator rbegin() const {
        return data.rbegin();
    }
    constexpr ConstReverseIterator rend() const {
        return data.rend();
    }
    constexpr ConstReverseIterator crbegin() const {
        return data.rbegin();
    }
    constexpr ConstReverseIterator crend() const {
        return data.rend();
    }

    constexpr auto empty() const -> bool {
        return data.size() == 0;
    }
    constexpr auto size() const -> SizeT {
        return data.size();
    }
    constexpr auto capacity() const -> SizeT {
        return data.size();
    }

    constexpr auto slot_iter_from_data_iter(ConstIterator data_iter) -> SlotIterator {
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
