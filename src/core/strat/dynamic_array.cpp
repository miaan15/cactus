module;

export module cactus.core.strat:dynamic_array;

import std;
import :assert;

using size_t = std::size_t;
namespace stdr = std::ranges;

namespace cact {

export template <typename T, typename Alloc = std::allocator<T>>
requires std::is_trivially_copyable_v<T>
struct DynamicArray {
    using alloc_traits_t = std::allocator_traits<Alloc>;

    // =========================================================================
    T *data = nullptr;
    size_t len = 0;
    size_t cap = 0;

    [[no_unique_address]] Alloc allocator = Alloc();

    // =========================================================================
    [[nodiscard]] static DynamicArray make() noexcept {
        return DynamicArray{};
    }

    void destroy() noexcept {
        if (data) { alloc_traits_t::deallocate(allocator, data, cap); }
    }

    [[nodiscard]] DynamicArray clone() const noexcept {
        if (cap == 0) return DynamicArray::make();

        Alloc new_allocator = Alloc(allocator);
        T *new_data = alloc_traits_t::allocate(new_allocator, cap);
        std::memcpy(new_data, data, len * sizeof(T));

        return DynamicArray{
            .data = new_data,
            .len = len,
            .cap = cap,
            .allocator = new_allocator};
    }

    void relinquish() noexcept {
        data = nullptr;
        len = 0;
        cap = 0;
    }

    // =============================================================================
    void reserve(size_t new_cap) noexcept {
        if (new_cap <= cap) return;

        T *new_data = alloc_traits_t::allocate(allocator, new_cap);

        if (data) {
            std::memcpy(new_data, data, len * sizeof(T));
            alloc_traits_t::deallocate(allocator, data, cap);
        }

        data = new_data;
        cap = new_cap;
    }

    void resize(size_t new_size) noexcept {
        if (new_size > cap) { grow(new_size); }
        len = new_size;
    }

    void append(const T &val) noexcept {
        if (len == cap) { grow(len + 1); }
        data[len] = val;
        ++len;
    }

    template <stdr::input_range R>
    requires std::convertible_to<stdr::range_reference_t<R>, T>
    void append(R&& range) noexcept {
        if constexpr (stdr::sized_range<R>) {
            size_t range_len = stdr::size(range);
            if (len + range_len > cap) {
                grow(len + range_len);
            }

            if constexpr (stdr::contiguous_range<R>) {
                std::memcpy(data + len,
                            stdr::data(range),
                            range_len * sizeof(T));
            } else {
                stdr::copy(range, data + len);
            }

            len += range_len;
        } else {
            for (auto&& val : range) {
                append(val);
            }
        }
    }

    template <std::input_iterator It, std::sentinel_for<It> S>
    requires std::convertible_to<std::iter_reference_t<It>, T>
    void append(It first, S last) noexcept {
        append(stdr::subrange(first, last));
    }

    void pop() noexcept {
        if (len > 0) --len;
    }

    bool set(size_t index, const T &val) noexcept {
        if (index > len) return false;
        data[index] = val;
        return true;
    }

    [[nodiscard]] std::optional<T> get(size_t index) const noexcept {
        if (index >= len) return {};
        return data[index];
    }

    [[nodiscard]] T * get_ptr(size_t index) noexcept {
        if (index >= len) return nullptr;
        return &data[index];
    }
    [[nodiscard]] const T * get_ptr(size_t index) const noexcept {
        if (index >= len) return nullptr;
        return &data[index];
    }

    void clear() noexcept { len = 0; }

    using iterator = T *;
    using const_iterator = const T *;

    iterator begin()  { return data; }
    iterator end() { return data + len; }
    const_iterator begin() const { return data; }
    const_iterator end() const { return data + len; }
    const_iterator cbegin() const { return data; }
    const_iterator cend() const { return data + len; }

    [[nodiscard]] bool empty() const noexcept { return len == 0; }

    void grow(size_t min_cap) noexcept {
        size_t new_cap = cap < 4 ? 4 : cap + (cap / 2);
        if (new_cap < min_cap) new_cap = min_cap;

        reserve(new_cap);
    }
};

} // namespace cact
