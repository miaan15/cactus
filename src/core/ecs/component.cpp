module;

#include <cassert>

export module cactus.core.ecs:component;

import std;

using size_t = std::size_t;

namespace cactus {

export struct ComponentSizeAlignData {
    size_t size;
    size_t align;
};

export struct ComponentSizeAlignAtlas {
    std::flat_map<std::type_index, ComponentSizeAlignData> component_size_align_data;

    [[nodiscard]] static auto make() -> ComponentSizeAlignAtlas { return ComponentSizeAlignAtlas{}; }

    auto destroy() const {}

    template <typename T> auto push() {
        component_size_align_data.insert_or_assign(std::type_index(typeid(T)), ComponentSizeAlignData{sizeof(T), alignof(T)});
    }

    template <typename T> [[nodiscard]] auto has() -> bool {
        return component_size_align_data.contains(std::type_index(typeid(T)));
    }
    [[nodiscard]] auto has(std::type_index component) -> bool { return component_size_align_data.contains(component); }

    template <typename T> [[nodiscard]] auto get_size_align() -> ComponentSizeAlignData {
        auto f = component_size_align_data.find(std::type_index(typeid(T)));
        assert(f != component_size_align_data.end());
        return f->second;
    }
    [[nodiscard]] auto get_size_align(std::type_index component) -> ComponentSizeAlignData {
        auto f = component_size_align_data.find(component);
        assert(f != component_size_align_data.end());
        return f->second;
    }
};

} // namespace cactus
