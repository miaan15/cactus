module;

export module cactus.core.strat:dynamic_array;

import std;
import :assert;

using size_t = std::size_t;

namespace cactus {

export template <typename T, typename Alloc = std::allocator<T>>
    requires std::is_trivially_copyable_v<T>
struct DynamicArray {
    using AllocTraits = std::allocator_traits<Alloc>;

    T *data = nullptr;
    size_t size = 0;
    size_t cap = 0;

    [[no_unique_address]] Alloc allocator;

    static auto make() -> DynamicArray { return DynamicArray{.data = nullptr, .size = 0, .cap = 0, .allocator = Alloc()}; }
    auto destroy() { AllocTraits::deallocate(allocator, data, cap); }

    auto append(const T &val) noexcept {
        if (size == cap) { grow(size + 1); }
        data[size++] = val;
    }

    auto resize(size_t new_size) noexcept {
        if (new_size > cap) { grow(new_size); }
        size = new_size;
    }

    auto reserve(size_t new_cap) noexcept {
        if (new_cap <= cap) return;

        T *new_data = AllocTraits::allocate(allocator, new_cap);

        if (data) {
            std::memcpy(new_data, data, size * sizeof(T));
            AllocTraits::deallocate(allocator, data, cap);
        }

        data = new_data;
        cap = new_cap;
    }

    auto operator[](size_t index) noexcept -> T & {
        _assert(index < size, "Index out of bounds");
        return data[index];
    }

    auto operator[](size_t index) const noexcept -> const T & {
        _assert(index < size, "Index out of bounds");
        return data[index];
    }

    auto pop() noexcept {
        if (size > 0) --size;
    }

    auto clear() noexcept { size = 0; }

    auto begin() noexcept -> T * { return data; }
    auto end() noexcept -> T * { return data + size; }
    auto begin() const noexcept -> const T * { return data; }
    auto end() const noexcept -> const T * { return data + size; }

    auto is_empty() const noexcept -> bool { return size == 0; }

    auto grow(size_t min_cap) noexcept {
        size_t new_cap = cap == 0 ? 4 : cap + (cap / 2);
        if (new_cap < min_cap) new_cap = min_cap;

        reserve(new_cap);
    }
};

} // namespace cactus
