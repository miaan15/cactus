module;

#include <boost/container/vector.hpp>
#include <optional>

export module FreelistVector;

namespace cactus {

export template <typename T, template <typename...> typename Container = boost::container::vector>
    requires std::is_same<typename Container<T>::value_type, T>::value
struct FreelistVector {
    using value_type = T;
    using container_type = Container<value_type>;
    using size_type = typename container_type::size_type;
    using difference_type = typename container_type::difference_type;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;

    union data_value_type {
        value_type value;
        size_type next;

        template <typename... Args>
        data_value_type(Args &&...args) : value(std::forward<Args>(args)...) {}
        data_value_type() : next(0) {}

        ~data_value_type() {
            value.~value_type();
        }

        operator value_type() const {
            return value;
        }
    };
    using data_container_type = Container<data_value_type>;
    using valid_lookup_type = boost::container::vector<bool>;

    data_container_type data;
    valid_lookup_type valid_lookup;
    size_type next_free_index = size_type(-1);

    [[nodiscard]]
    constexpr auto at(size_type index) -> std::optional<pointer> {
        if (index >= data.size()) return {};
        if (!valid_lookup[index]) return {};
        auto iter = std::next(data.begin(), index);
        return &iter->value;
    }
    [[nodiscard]]
    constexpr auto at(size_type index) const -> std::optional<const_pointer> {
        if (index >= data.size()) return {};
        if (!valid_lookup[index]) return {};
        auto iter = std::next(data.begin(), index);
        return &iter->value;
    }

    [[nodiscard]]
    constexpr auto operator[](size_type index) -> reference {
        auto iter = std::next(data.begin(), index);
        return iter->value;
    }
    [[nodiscard]]
    constexpr auto operator[](size_type index) const -> const_reference {
        auto iter = std::next(data.begin(), index);
        return iter->value;
    }

    [[nodiscard]]
    constexpr auto contains(size_type index) const -> bool {
        return index < data.size() && valid_lookup[index];
    }

    template <class... Args>
    constexpr auto emplace(Args &&...args) -> size_type {
        if (next_free_index == size_type(-1)) {
            data.emplace_back(std::forward<Args>(args)...);
            valid_lookup.emplace_back(true);
            return data.size() - 1;
        } else {
            auto index = next_free_index;
            next_free_index = data[next_free_index].next;
            auto iter = std::next(data.begin(), index);
            new (std::addressof(iter->value)) value_type(std::forward<Args>(args)...);
            valid_lookup[index] = true;
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
        if (index >= data.size()) return;
        if (!valid_lookup[index]) return;
        valid_lookup[index] = false;
        auto iter = std::next(data.begin(), index);
        iter->value.~value_type();
        new (std::addressof(iter->next)) size_type(next_free_index);
        next_free_index = index;
    }

    constexpr auto reserve(size_type n) -> void {
        data.reserve(n);
        valid_lookup.reserve(n);
    }

    constexpr auto clear() -> void {
        data.clear();
        valid_lookup.clear();
        next_free_index = size_type(-1);
    }

    constexpr auto swap(FreelistVector &rhs) -> void {
        using std::swap;
        swap(data, rhs.data);
        swap(valid_lookup, rhs.valid_lookup);
        swap(next_free_index, rhs.next_free_index);
    }

    constexpr auto size() const -> size_type {
        return data.size();
    }
    constexpr auto capacity() const -> size_type {
        return data.capacity();
    }

    template <bool IsConst>
    struct base_iterator {
        using root_ptr = std::conditional_t<IsConst, const FreelistVector *, FreelistVector *>;
        using ref_type = std::conditional_t<IsConst, const_reference, reference>;
        using ptr_type = std::conditional_t<IsConst, const_pointer, pointer>;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = typename FreelistVector::difference_type;
        using pointer = ptr_type;
        using reference = ref_type;

        root_ptr container;
        size_type index;

        operator base_iterator<true>() const
            requires(!IsConst)
        {
            return {container, index};
        }

        base_iterator &operator++() {
            ++index;
            while (index < container->data.size() && !container->valid_lookup[index]) ++index;
            return *this;
        }
        base_iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        base_iterator &operator--() {
            --index;
            while (index != size_type(-1) && !container->valid_lookup[index]) --index;
            return *this;
        }
        base_iterator operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        ref_type operator*() const {
            return container->data[index].value;
        }
        ptr_type operator->() const {
            return &container->data[index].value;
        }

        bool operator==(const base_iterator &) const = default;
    };

    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr auto begin() -> iterator {
        size_type i = 0;
        while (i < data.size() && !valid_lookup[i]) ++i;
        return {this, i};
    }
    constexpr auto end() -> iterator {
        return {this, data.size()};
    }
    constexpr auto begin() const -> const_iterator {
        size_type i = 0;
        while (i < data.size() && !valid_lookup[i]) ++i;
        return {this, i};
    }
    constexpr auto end() const -> const_iterator {
        return {this, data.size()};
    }
    constexpr auto cbegin() const -> const_iterator {
        return begin();
    }
    constexpr auto cend() const -> const_iterator {
        return end();
    }
    constexpr auto rbegin() -> reverse_iterator {
        return reverse_iterator(end());
    }
    constexpr auto rend() -> reverse_iterator {
        return reverse_iterator(begin());
    }
    constexpr auto rbegin() const -> const_reverse_iterator {
        return const_reverse_iterator(end());
    }
    constexpr auto rend() const -> const_reverse_iterator {
        return const_reverse_iterator(begin());
    }
    constexpr auto crbegin() const -> const_reverse_iterator {
        return rbegin();
    }
    constexpr auto crend() const -> const_reverse_iterator {
        return rend();
    }
};

export template <class T, template <class...> class Container>
constexpr void swap(FreelistVector<T, Container> &lhs, FreelistVector<T, Container> &rhs) {
    lhs.swap(rhs);
}

} // namespace cactus
