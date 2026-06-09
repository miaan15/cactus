module;

export module cactus.common;

export import std;

export namespace stdf = std::filesystem;
export namespace stdr = std::ranges;
export namespace stdv = std::views;

export using size_t = std::size_t;

export using i8  = std::int8_t;
export using i16 = std::int16_t;
export using i32 = std::int32_t;
export using i64 = std::int64_t;
export using iptr = std::intptr_t;

export using u8  = std::uint8_t;
export using u16 = std::uint16_t;
export using u32 = std::uint32_t;
export using u64 = std::uint64_t;
export using uptr = std::uintptr_t;

namespace cactus {

export const stdf::path _root_dir = stdf::path{__FILE__}.parent_path() / "..";
export const stdf::path _src_dir = _root_dir / "src";

} // namespace cactus
