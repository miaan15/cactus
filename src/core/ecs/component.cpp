module;

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

    template <typename T> auto push() {
        component_size_align_data.insert_or_assign(std::type_index(typeid(T)),
                                                       ComponentSizeAlignData{sizeof(T), alignof(T)});
    }

    template <typename T> [[nodiscard]] auto has() -> bool {
        return component_size_align_data.contains(std::type_index(typeid(T)));
    }

    template <typename T> [[nodiscard]] auto get_size_align() -> std::optional<ComponentSizeAlignData> {
        auto f = component_size_align_data.find(std::type_index(typeid(T)));
        if (f == component_size_align_data.end()) return {};
        return f->second;
    }
    [[nodiscard]] auto get_size_align(std::type_index component) -> std::optional<ComponentSizeAlignData> {
        auto f = component_size_align_data.find(component);
        if (f == component_size_align_data.end()) return {};
        return f->second;
    }
};

} // namespace cactus
