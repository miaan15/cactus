module;

#include <filesystem>

export module Common;

namespace stdf = std::filesystem;

namespace cactus {

export const stdf::path root_dir = stdf::path{__FILE__}.parent_path() / "..";
export const stdf::path src_dir = root_dir / "src";

}
