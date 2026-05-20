module;

export module cactus.core.strat:fixedarr;

import std;

using size_t = std::size_t;

namespace cactus {

export template <typename T, typename Alloc = std::allocator<T>> struct FixedArr {
    using AllocTraits = std::allocator_traits<Alloc>;

    T *data_raw;
    size_t len;

    [[no_unique_address]] Alloc allocator;

    explicit FixedArr(size_t len) : len(len) {
        allocator = Alloc();
        data_raw = AllocTraits::allocate(allocator, len);
    }

    ~FixedArr() {
        if (data_raw != nullptr) AllocTraits::deallocate(allocator, data_raw, len);
    }

    FixedArr(const FixedArr &other) = delete;
    FixedArr &operator=(const FixedArr &other) = delete;

    FixedArr(FixedArr &&other) noexcept : data_raw(other.data_raw), len(other.len), allocator(std::move(other.allocator)) {
        other.data_raw = nullptr;
        other.len = 0;
    }
    FixedArr &operator=(FixedArr &&other) noexcept {
        if (this != &other) {
            std::swap(data_raw, other.data_raw);
            std::swap(len, other.len);
            std::swap(allocator, other.allocator);
        }
        return *this;
    }

    auto set(size_t index, const T &val) -> bool {
        if (index >= len) return false;
        data_raw[index] = val;
        return true;
    }

    [[nodiscard]] auto get(size_t index) const -> std::optional<T> {
        if (index >= len) return {};
        return data_raw[index];
    }

    [[nodiscard]] auto get_ptr(size_t index) -> T * {
        if (index >= len) return {};
        return &data_raw[index];
    }
    [[nodiscard]] auto get_ptr(size_t index) const -> const T * {
        if (index >= len) return {};
        return &data_raw[index];
    }

    [[nodiscard]] auto has(size_t index) const -> bool { return index < len; }
};

} // namespace cactus
