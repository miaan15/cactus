module;

export module cactus.core.strat:fixed_array;

import std;

using size_t = std::size_t;

namespace cact {

export template <typename T, typename Alloc = std::allocator<T>> \
struct FixedArray {
    using alloc_traits_t = std::allocator_traits<Alloc>;

    // =========================================================================
    T *data = nullptr;
    size_t len = 0;

    [[no_unique_address]] Alloc allocator = Alloc();

    // =========================================================================
    [[nodiscard]] static FixedArray make(size_t len) noexcept {
        Alloc allocator = Alloc();
        T *data = alloc_traits_t::allocate(allocator, len);

        std::memset(data, 0, len * sizeof(T));

        return FixedArray{ 
            .data = data, 
            .len = len,
            .allocator = allocator};
    }

    void destroy() noexcept {
        if (data) alloc_traits_t::deallocate(allocator, data, len);
    }

    [[nodiscard]] FixedArray clone() const noexcept {
        Alloc new_allocator = Alloc(allocator);
        T *new_data = alloc_traits_t::allocate(new_allocator, len);

        std::memcpy(new_data, data, len * sizeof(T));

        return FixedArray{
            .data = new_data,
            .len = len,
            .allocator = new_allocator};
    }

    void relinquish() noexcept {
        data = nullptr;
        len = 0;
    }

    // =========================================================================
    void remake(size_t new_len) noexcept {
        size_t copy_len = new_len < len ? new_len : len;
        T *new_data = alloc_traits_t::allocate(allocator, new_len);

        std::memcpy(new_data, data, copy_len * sizeof(T));

        if (new_len > len) { 
            std::memset(new_data + len, 0, (new_len - len) * sizeof(T)); 
        }

        if (data) alloc_traits_t::deallocate(allocator, data, len);

        data = new_data;
        len = new_len;
    }

    bool set(size_t index, const T &val) noexcept {
        if (index >= len) return false;
        data[index] = val;
        return true;
    }

    [[nodiscard]] std::optional<T> get(size_t index) const noexcept {
        if (index >= len) return {};
        return data[index];
    }

    [[nodiscard]] T * get_ptr(size_t index) noexcept {
        if (index >= len) return {};
        return &data[index];
    }
    [[nodiscard]] const T * get_ptr(size_t index) const noexcept {
        if (index >= len) return {};
        return &data[index];
    }

    using iterator = T *;
    using const_iterator = const T *;

    iterator begin() { return data; }
    iterator end() { return data + len; }
    const_iterator begin() const { return data; }
    const_iterator end() const { return data + len; }
    const_iterator cbegin() const { return data; }
    const_iterator cend() const { return data + len; }
};

} // namespace cact
