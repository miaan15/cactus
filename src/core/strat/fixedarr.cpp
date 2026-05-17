module;

export module cactus.core.strat:fixedarr;

import std;

using size_t = std::size_t;

namespace cactus {

template <typename T, typename Alloc = std::allocator<T>> struct FixedArr {
    using AllocTraits = std::allocator_traits<Alloc>;

    T *data_raw;
    size_t len;

    [[no_unique_address]] Alloc allocator;

    [[nodiscard]] static auto make(size_t len) -> FixedArr {
        auto alloc = Alloc();
        T *data_raw = AllocTraits::allocate(Alloc(), len);
        return FixedArr{.data_raw = data_raw, .len = len, .allocator = alloc};
    }

    auto destroy() const { AllocTraits::deallocate(allocator, data_raw, len); }

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
