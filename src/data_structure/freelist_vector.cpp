module;

#include <boost/container/vector.hpp>
#include <optional>

export module FreelistVector;

namespace cactus {

export template <typename T, template <typename...> typename Container = boost::container::vector>
    requires std::is_same<typename Container<T>::value_type, T>::value
             && std::is_trivially_copyable_v<T>
struct FreelistVector {
    using value_type = T;
    using container_type = Container<T>;
    using size_type = typename container_type::size_type;
    using difference_type = typename container_type::difference_type;

    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;

    struct data_element_type {
        union {
            value_type value;
            size_type next;
        };

        bool valid;
    };
    using data_container_type = Container<data_element_type>;
    using data_reference = typename data_container_type::reference;
    using data_pointer = typename data_container_type::pointer;
    using const_data_pointer = typename data_container_type::const_pointer;
    using data_iterator = typename data_container_type::iterator;
    using const_data_iterator = typename data_container_type::const_iterator;
    using reverse_data_iterator = typename data_container_type::reverse_iterator;
    using const_reverse_data_iterator = typename data_container_type::const_reverse_iterator;

    template <typename _Pointer, typename _Reference, typename _Data>
    struct _iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = _Data::difference_type;
        using size_type = _Data::size_type;
        using pointer = _Pointer;
        using reference = _Reference;

        _iterator(_Data *data, size_type index) : _data(data), _index(index) {}

        reference operator*() const {
            return _data->data()[_index].value;
        }
        pointer operator->() {
            return &_data->data()[_index].value;
        }

        _iterator &operator++() {
            ++_index;
            while (!_data->data()[_index].valid && _index < _data->size()) ++_index;
            return *this;
        }
        _iterator operator++(int) {
            _iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        _iterator &operator--() {
            --_index;
            while (!_data->data()[_index].valid) --_index;
            return *this;
        }
        _iterator operator--(int) {
            _iterator tmp = *this;
            --(*this);
            return tmp;
        }

        friend bool operator==(const _iterator &a, const _iterator &b) {
            return a._data->data() == b._data->data() && a._index == b._index;
        };

        _Data *_data;
        size_type _index;
    };

    using iterator = _iterator<pointer, reference, data_container_type>;
    using const_iterator = _iterator<const_pointer, const_reference, const data_container_type>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    data_container_type data;
    size_type next_free_index = size_type(-1);

    [[nodiscard]] constexpr auto at(size_type index) -> std::optional<pointer> {
        if (index >= data.size()) return {};
        auto data_iter = std::next(data.begin(), index);
        if (!data_iter->valid) return {};
        return &data_iter->value;
    }
    [[nodiscard]] constexpr auto at(size_type index) const -> std::optional<const_pointer> {
        if (index >= data.size()) return {};
        auto data_iter = std::next(data.begin(), index);
        if (!data_iter->valid) return {};
        return &data_iter->value;
    }

    [[nodiscard]] constexpr auto operator[](size_type index) -> reference {
        auto data_iter = std::next(data.begin(), index);
        return data_iter->value;
    }
    [[nodiscard]] constexpr auto operator[](size_type index) const -> const_reference {
        auto data_iter = std::next(data.begin(), index);
        return data_iter->value;
    }

    template <class... Args>
    constexpr auto emplace(Args &&...args) -> size_type {
        if (next_free_index == size_type(-1)) {
            auto x = data_element_type{.valid = true};
            x.value = {std::forward<Args>(args)...};
            data.emplace_back(x);
            return data.size() - 1;
        } else {
            auto index = next_free_index;
            next_free_index = data[next_free_index].next;
            auto data_iter = std::next(data.begin(), index);
            data_iter->value = {std::forward<Args>(args)...};
            data_iter->valid = true;
            return index;
        }
    }

    constexpr auto insert(const value_type &value) -> size_type {
        return this->emplace(value);
    }
    constexpr auto insert(value_type &&value) -> size_type {
        return this->emplace(std::move(value));
    }

    constexpr auto erase(size_type index) {
        auto data_iter = std::next(data.begin(), index);
        if (!data_iter->valid) return;
        data_iter->next = next_free_index;
        data_iter->valid = false;
        next_free_index = index;
    }

    constexpr auto erase(iterator pos) {
        this->erase(static_cast<size_type>(pos._index));
    }

    constexpr auto erase(iterator first, iterator last) {
        auto first_index = first._index - this->begin()._index;
        auto last_index = last._index - this->begin()._index;
        while (last_index != first_index) {
            --last_index;
            this->erase(last_index);
        }
    }

    constexpr auto reserve(size_type n) -> void {
        data.reserve(n);
    }

    constexpr auto clear() -> void {
        data.clear();
        next_free_index = size_type(-1);
    }

    constexpr auto swap(FreelistVector &rhs) -> void {
        using std::swap;
        swap(data, rhs.data);
        swap(next_free_index, rhs.next_free_index);
    }

    constexpr iterator begin() {
        size_type i = 0;
        while (i < data.size() && !data[i].valid) ++i;
        return iterator(&data, i);
    }
    constexpr iterator end() {
        return iterator(&data, data.size());
    }
    constexpr const_iterator begin() const {
        size_type i = 0;
        while (i < data.size() && !data[i].valid) ++i;
        return const_iterator(&data, i);
    }
    constexpr const_iterator end() const {
        return const_iterator(&data, data.size());
    }
    constexpr const_iterator cbegin() const {
        size_type i = 0;
        while (i < data.size() && !data[i].valid) ++i;
        return const_iterator(&data, i);
    }
    constexpr const_iterator cend() const {
        return const_iterator(&data, data.size());
    }
    constexpr reverse_iterator rbegin() {
        return reverse_iterator(end());
    }
    constexpr reverse_iterator rend() {
        return reverse_iterator(begin());
    }
    constexpr const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }
    constexpr const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }
    constexpr const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }
    constexpr const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    constexpr auto capacity() const -> size_type {
        return data.capacity();
    }
};

export template <class T, template <class...> class Container>
constexpr void swap(FreelistVector<T, Container> &lhs, FreelistVector<T, Container> &rhs) {
    lhs.swap(rhs);
}

}; // namespace cactus
