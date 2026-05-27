module;

export module cactus.core.strat:dynamic_array;

import std;
import :assert;

using size_t = std::size_t;

namespace cactus {

export template <typename T, typename Alloc = std::allocator<T>>
    requires std::is_trivially_copyable_v<T>
struct DynamicArray {
    using alloc_traits_t = std::allocator_traits<Alloc>;

    T *data = nullptr;
    size_t len = 0;
    size_t cap = 0;

    [[no_unique_address]] Alloc allocator = Alloc();

    [[nodiscard]] static auto make() noexcept -> DynamicArray { return DynamicArray{}; }
    auto destroy() noexcept {
        if (data) { alloc_traits_t::deallocate(allocator, data, cap); }
    }
    [[nodiscard]] auto clone() noexcept -> DynamicArray {
        if (cap == 0) return DynamicArray::make();

        T *new_data = alloc_traits_t::allocate(allocator, cap);
        memcpy(new_data, data, len * sizeof(T));

        return DynamicArray{.data = new_data, .len = len, .cap = cap, .allocator = allocator};
    }

    auto reserve(size_t new_cap) noexcept {
        if (new_cap <= cap) return;

        T *new_data = alloc_traits_t::allocate(allocator, new_cap);

        if (data) {
            std::memcpy(new_data, data, len * sizeof(T));
            alloc_traits_t::deallocate(allocator, data, cap);
        }

        data = new_data;
        cap = new_cap;
    }

    auto resize(size_t new_size) noexcept {
        if (new_size > cap) { grow(new_size); }
        len = new_size;
    }
    auto append(const T &val) noexcept {
        if (len == cap) { grow(len + 1); }
        data[len] = val;
        ++len;
    }

    auto pop() noexcept {
        if (len > 0) --len;
    }

    auto set(size_t index, const T &val) noexcept -> bool {
        if (index > len) return false;
        data[index] = val;
        return true;
    }

    [[nodiscard]] auto get(size_t index) const noexcept -> std::optional<T> {
        if (index >= len) return {};
        return data[index];
    }

    [[nodiscard]] auto get_ptr(size_t index) noexcept -> T * {
        if (index >= len) return nullptr;
        return &data[index];
    }
    [[nodiscard]] auto get_ptr(size_t index) const noexcept -> const T * {
        if (index >= len) return nullptr;
        return &data[index];
    }

    auto clear() noexcept { len = 0; }

    auto begin() noexcept -> T * { return data; }
    auto end() noexcept -> T * { return data + len; }
    auto begin() const noexcept -> const T * { return data; }
    auto end() const noexcept -> const T * { return data + len; }

    auto empty() const noexcept -> bool { return len == 0; }

    auto grow(size_t min_cap) noexcept {
        size_t new_cap = cap == 0 ? 4 : cap + (cap / 2);
        if (new_cap < min_cap) new_cap = min_cap;

        reserve(new_cap);
    }
};

} // namespace cactus
