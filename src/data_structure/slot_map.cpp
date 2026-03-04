module;

#include <boost/container/vector.hpp>
#include <iterator>
#include <optional>
#include <tuplet/tuple.hpp>

export module SlotMap;

namespace cactus {

export template <typename T, typename GenT = size_t,
                 template <typename...> typename Container = boost::container::vector,
                 template <typename...> typename SlotContainer = boost::container::vector>
    requires std::is_same_v<typename Container<T>::value_type, T>
             && std::is_same_v<typename Container<T>::size_type,
                               typename SlotContainer<typename Container<T>::size_type>::size_type>
struct SlotMap {
    using value_type = T;
    using container_type = Container<value_type>;
    using size_type = typename container_type::size_type;
    using difference_type = typename container_type::difference_type;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;

    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;

    using generation_type = GenT;

    struct key_type {
        size_type index;
        generation_type gen;

        static constexpr key_type from(const tuplet::tuple<size_t, size_t> &t) {
            return {tuplet::get<0>(t), tuplet::get<1>(t)};
        }

        operator tuplet::tuple<size_t, size_t>() const {
            return {index, gen};
        }

        constexpr auto operator<=>(const key_type &other) const = default;
    };
    using key_tuple_type = tuplet::tuple<size_type, generation_type>;

    using data_map_container = Container<size_type>;

    using slot_type = key_type;
    using slot_container_type = SlotContainer<slot_type>;

    container_type data;
    data_map_container data_map;
    slot_container_type slots;
    size_type next_slot_index = 0;

    using slot_iterator = typename decltype(slots)::iterator;

    [[nodiscard]] constexpr auto find(const key_type &key) -> iterator {
        auto index = key.index;
        if (index >= slots.size()) return end();

        auto slot_iter = std::next(slots.begin(), index);
        if (slot_iter->gen != key.gen) return end();

        return std::next(data.begin(), slot_iter->index);
    }
    [[nodiscard]] constexpr auto find(const key_type &key) const -> const_iterator {
        auto index = key.index;
        if (index >= slots.size()) return end();

        auto slot_iter = std::next(slots.begin(), index);
        if (slot_iter->gen != key.gen) return end();

        return std::next(data.begin(), slot_iter->index);
    }
    [[nodiscard]] constexpr auto find(const key_tuple_type &key) -> iterator {
        return find(key_type::from(key));
    }
    [[nodiscard]] constexpr auto find(const key_tuple_type &key) const -> const_iterator {
        return find(key_type::from(key));
    }

    [[nodiscard]] constexpr auto at(const key_type &key) -> reference {
        auto data_iter = this->find(key);
        if (data_iter == this->end()) throw std::out_of_range("SlotMap::at: invalid key");
        return *data_iter;
    }
    [[nodiscard]] constexpr auto at(const key_type &key) const -> const_reference {
        auto data_iter = this->find(key);
        if (data_iter == this->end()) throw std::out_of_range("SlotMap::at: invalid key");
        return *data_iter;
    }
    [[nodiscard]] constexpr auto at(const key_tuple_type &key) -> reference {
        return at(key_type::from(key));
    }
    [[nodiscard]] constexpr auto at(const key_tuple_type &key) const -> const_reference {
        return at(key_type::from(key));
    }

    [[nodiscard]] constexpr auto operator[](const key_type &key) -> reference {
        auto slot_iter = std::next(slots.begin(), key.index);
        return *std::next(data.begin(), slot_iter->index);
    }
    [[nodiscard]] constexpr auto operator[](const key_type &key) const -> const_reference {
        auto slot_iter = std::next(slots.begin(), key.index);
        return *std::next(data.begin(), slot_iter->index);
    }
    [[nodiscard]] constexpr auto operator[](const key_tuple_type &key) -> reference {
        return operator[](key_type::from(key));
    }
    [[nodiscard]] constexpr auto operator[](const key_tuple_type &key) const -> const_reference {
        return operator[](key_type::from(key));
    }

    [[nodiscard]] constexpr auto contains(const key_type &key) const -> bool {
        auto index = key.index;
        if (index >= slots.size()) return false;

        auto slot_iter = std::next(slots.begin(), index);
        if (slot_iter->gen != key.gen) return false;

        return true;
    }
    [[nodiscard]] constexpr auto contains(const key_tuple_type &key) const -> bool {
        return contains(key_type::from(key));
    }

    [[nodiscard]] constexpr auto get(const key_type &key) -> std::optional<pointer> {
        auto data_iter = this->find(key);
        if (data_iter == this->end()) return {};
        return &*data_iter;
    }
    [[nodiscard]] constexpr auto get(const key_type &key) const -> std::optional<const_pointer> {
        auto data_iter = this->find(key);
        if (data_iter == this->end()) return {};
        return &*data_iter;
    }
    [[nodiscard]] constexpr auto get(const key_tuple_type &key) -> std::optional<pointer> {
        return get(key_type::from(key));
    }
    [[nodiscard]] constexpr auto get(const key_tuple_type &key) const
        -> std::optional<const_pointer> {
        return get(key_type::from(key));
    }

    template <class... Args>
    constexpr auto emplace(Args &&...args) -> key_type {
        data.emplace_back(std::forward<Args>(args)...);
        data_map.emplace_back(next_slot_index);

        if (next_slot_index == slots.size()) {
            slots.emplace_back(next_slot_index + 1, generation_type{});
            // set index = next_slot_index + 1 for later next_slot_index assignment
        }

        auto slot_iter = std::next(slots.begin(), next_slot_index);
        next_slot_index = slot_iter->index;
        slot_iter->index = data.size() - 1;

        key_type res = *slot_iter;
        res.index = std::distance(slots.begin(), slot_iter);
        return res;
    }

    constexpr auto insert(const value_type &value) -> key_type {
        return this->emplace(value);
    }
    constexpr auto insert(value_type &&value) -> key_type {
        return this->emplace(std::move(value));
    }

    constexpr auto erase(const_iterator iter) -> iterator {
        auto slot_iter = slot_iter_from_data_iter(iter);
        auto data_index = slot_iter->index;
        auto data_iter = std::next(data.begin(), data_index);
        auto last_data_iter = std::prev(data.end());

        if (data_iter != last_data_iter) {
            auto last_data_slot_iter = slot_iter_from_data_iter(last_data_iter);

            *data_iter = std::move(*last_data_iter);

            last_data_slot_iter->index = data_index;

            *std::next(data_map.begin(), data_index) =
                std::distance(slots.begin(), last_data_slot_iter);
        }

        data.pop_back();
        data_map.pop_back();

        slot_iter->index = next_slot_index;
        next_slot_index = std::distance(slots.begin(), slot_iter);

        ++slot_iter->gen;
        return std::next(data.begin(), data_index);
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
    constexpr auto erase(const key_type &key) -> iterator {
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
        data.clear();
        data_map.clear();
        slots.clear();
        next_slot_index = 0;
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
        return data.empty();
    }
    constexpr auto size() const -> size_type {
        return data.size();
    }
    constexpr auto capacity() const -> size_type {
        return data.capacity();
    }

    constexpr void swap(SlotMap &other) noexcept {
        using std::swap;
        swap(data, other.data);
        swap(data_map, other.data_map);
        swap(slots, other.slots);
        swap(next_slot_index, other.next_slot_index);
    }

    friend constexpr void swap(SlotMap &lhs, SlotMap &rhs) noexcept {
        lhs.swap(rhs);
    }

    constexpr auto slot_iter_from_data_iter(const_iterator data_iter) -> slot_iterator {
        auto data_index = std::distance(const_iterator(data.begin()), data_iter);
        auto slot_index = *std::next(data_map.begin(), data_index);
        return std::next(slots.begin(), slot_index);
    }
};

} // namespace cactus
