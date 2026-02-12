module;

#include <boost/container/vector.hpp>
#include <optional>

export module Pool;

namespace cactus {

export template <typename T, template <typename...> typename Container = boost::container::vector>
    requires std::is_same<typename Container<T>::value_type, T>::value
             && std::is_trivially_copyable_v<T>
struct Pool {
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

    template <typename _Pointer, typename _Reference, typename _DataPointer>
    struct _iterator {
        using iterator_category = std::random_access_iterator_tag;

        using value_type = std::remove_cv_t<std::remove_pointer_t<_Pointer>>;
        using size_type = size_type;
        using difference_type = difference_type;
        using pointer = _Pointer;
        using reference = _Reference;

        _iterator(_DataPointer root, size_type index) : _root(root), _index(index) {}

        reference operator*() const {
            return _root[_index].value;
        }
        pointer operator->() {
            return &_root[_index].value;
        }

        _iterator &operator++() {
            ++_index;
            while (!_root[_index].valid) ++_index;
            return *this;
        }
        _iterator operator++(int) {
            _iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        _iterator &operator--() {
            --_index;
            while (!_root[_index].valid) --_index;
            return *this;
        }
        _iterator operator--(int) {
            _iterator tmp = *this;
            --(*this);
            return tmp;
        }

        friend bool operator==(const _iterator &a, const _iterator &b) {
            return a._root == b._root && a._index == b._index;
        };
        friend bool operator!=(const _iterator &a, const _iterator &b) {
            return !(a == b);
        };

        _DataPointer _root;
        size_type _index;
    };

    using iterator = _iterator<pointer, reference, data_pointer>;
    using const_iterator = _iterator<const_pointer, const_reference, const_data_pointer>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    data_container_type data;
    size_type next_free_index = size_type(-1);

    [[nodiscard]] constexpr auto at(size_type index) -> std::optional<value_type *> {
        if (index >= data.size()) return {};
        auto data_iter = std::next(data.begin(), index);
        if (!data_iter->valid) return {};
        return &data_iter->value;
    }
    [[nodiscard]] constexpr auto at(size_type index) const -> std::optional<const value_type *> {
        if (index >= data.size()) return {};
        auto data_iter = std::next(data.begin(), index);
        if (!data_iter->valid) return {};
        return &data_iter->value;
    }

    [[nodiscard]] constexpr auto operator[](size_type index) -> value_type & {
        auto data_iter = std::next(data.begin(), index);
        return data_iter->value;
    }
    [[nodiscard]] constexpr auto operator[](size_type index) const -> const value_type & {
        auto data_iter = std::next(data.begin(), index);
        return data_iter->value;
    }

    template <class... Args>
    constexpr auto emplace(Args &&...args) -> iterator {
        if (next_free_index == size_type(-1)) {
            auto x = data_element_type{.valid = true};
            x.value = {std::forward<Args>(args)...};
            data.emplace_back(x);
            return iterator(data.data(), data.size() - 1);
        } else {
            auto index = next_free_index;
            next_free_index = data[next_free_index].next;
            auto data_iter = std::next(data.begin(), index);
            data_iter->value = {std::forward<Args>(args)...};
            data_iter->valid = true;
            return iterator(data.data(), index);
        }
    }

    constexpr auto insert(const value_type &value) -> iterator {
        return this->emplace(value);
    }
    constexpr auto insert(value_type &&value) -> iterator {
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

    constexpr auto swap(Pool &rhs) -> void {
        using std::swap;
        swap(data, rhs.data);
        swap(next_free_index, rhs.next_free_index);
    }

    constexpr iterator begin() {
        return iterator(data.data(), 0);
    }
    constexpr iterator end() {
        return iterator(data.data(), data.size());
    }
    constexpr const_iterator begin() const {
        return const_iterator(data.data(), 0);
    }
    constexpr const_iterator end() const {
        return const_iterator(data.data(), data.size());
    }
    constexpr const_iterator cbegin() const {
        return const_iterator(data.data(), 0);
    }
    constexpr const_iterator cend() const {
        return const_iterator(data.data(), data.size());
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

    constexpr auto empty() const -> bool {
        return data.size() == 0;
    }
    constexpr auto size() const -> size_type {
        return data.size();
    }
    constexpr auto capacity() const -> size_type {
        return data.capacity();
    }
};

export template <class T, template <class...> class Container>
constexpr void swap(Pool<T, Container> &lhs, Pool<T, Container> &rhs) {
    lhs.swap(rhs);
}

}; // namespace cactus
