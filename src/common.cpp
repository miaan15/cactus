module;

export module cactus.common;

export import std;

export namespace stdf = std::filesystem;
export using size_t = std::size_t;

namespace cactus {

export const stdf::path _root_dir = stdf::path{__FILE__}.parent_path() / "..";
export const stdf::path _src_dir = _root_dir / "src";

} // namespace cactus
