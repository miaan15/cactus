module;

export module cactus.core.strat:dynamic_array;

import std;
import :assert;

using size_t = std::size_t;

namespace cactus {

export template <typename T>
    requires(std::is_trivially_copyable<T>())
struct DynamicArray {
    T *data = nullptr;
    size_t size = 0;
    size_t cap = 0;

    static auto make() -> DynamicArray { return DynamicArray{.data_raw = nullptr, .size = 0, .cap = 0}; }
    auto destroy() { std::free(data); }

    auto append(const T &val) noexcept {
        if (size == cap) { grow(size + 1); }
        data[size++] = val;
    }

    auto resize(size_t new_size) noexcept {
        if (new_size > cap) { grow(new_size); }
        size = new_size;
    }

    auto reserve(size_t new_cap) noexcept {
        T *new_data = (T *)(std::realloc(data, new_cap * sizeof(T)));

        if (!new_data) _assert(false, "DynamicArray failed to allocate more data");

        data = new_data;
        cap = new_cap;
    }

    auto operator[](size_t index) noexcept -> std::optional<T> {
        if (index >= size) return {};
        return data[index];
    }

    auto operator[](size_t index) const noexcept -> std::optional<const T> {
        if (index >= size) return {};
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
