module;

export module cactus.core.strat:fixed_array;

import std;

using size_t = std::size_t;

namespace cactus {

export template <typename T, typename Alloc = std::allocator<T>> struct FixedArray {
    using alloc_traits_t = std::allocator_traits<Alloc>;

    T *data = nullptr;
    size_t len = 0;

    [[no_unique_address]] Alloc allocator = Alloc();

    [[nodiscard]] static auto make(size_t len) noexcept -> FixedArray {
        Alloc allocator = Alloc();
        T *data = alloc_traits_t::allocate(allocator, len);
        std::memset(data, 0, len * sizeof(T));
        return FixedArray{.data = data, .len = len, .allocator = allocator};
    }
    auto destroy() noexcept {
        if (data) alloc_traits_t::deallocate(allocator, data, len);
    }
    [[nodiscard]] auto clone() const noexcept -> FixedArray {
        Alloc new_allocator = Alloc(allocator);
        T *new_data = alloc_traits_t::allocate(new_allocator, len);
        std::memcpy(new_data, data, len * sizeof(T));
        return FixedArray{.data = new_data, .len = len, .allocator = new_allocator};
    }

    auto remake(size_t new_len) noexcept -> void {
        size_t copy_len = new_len < len ? new_len : len;
        T *new_data = alloc_traits_t::allocate(allocator, new_len);
        std::memcpy(new_data, data, copy_len * sizeof(T));

        if (new_len > len) { std::memset(new_data + len, 0, (new_len - len) * sizeof(T)); }

        if (data) alloc_traits_t::deallocate(allocator, data, len);

        data = new_data;
        len = new_len;
    }

    auto set(size_t index, const T &val) noexcept -> bool {
        if (index >= len) return false;
        data[index] = val;
        return true;
    }

    [[nodiscard]] auto get(size_t index) const -> std::optional<T> {
        if (index >= len) return {};
        return data[index];
    }

    [[nodiscard]] auto get_ptr(size_t index) -> T * {
        if (index >= len) return {};
        return &data[index];
    }
    [[nodiscard]] auto get_ptr(size_t index) const -> const T * {
        if (index >= len) return {};
        return &data[index];
    }

    using iterator = T *;
    using const_iterator = const T *;

    auto begin() -> iterator { return data; }
    auto end() -> iterator { return data + len; }
    auto begin() const -> const_iterator { return data; }
    auto end() const -> const_iterator { return data + len; }
    auto cbegin() const -> const_iterator { return data; }
    auto cend() const -> const_iterator { return data + len; }
};

} // namespace cactus
