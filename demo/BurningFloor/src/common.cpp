module;

export module burningfloor.common;

export import cact;
export import sdl;
export import std;
export import glm;

export const stdf::path root_dir = stdf::path{__FILE__}.parent_path() / "..";
export const stdf::path asset_dir = root_dir / "asset";

export using namespace cact;
