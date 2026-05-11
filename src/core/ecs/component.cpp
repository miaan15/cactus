module;

#include <cassert>

export module cactus.core.ecs:component;

import std;

using size_t = std::size_t;

namespace cactus {

export constexpr size_t MAX_COMPONENT_COUNT = 256;

export using ComponentKey = size_t;

export struct ComponentData {
    size_t size;
    size_t align;
};

export struct ComponentAtlas {
    std::flat_map<std::type_index, ComponentKey> component_to_key_map;
    std::vector<ComponentData> component_datas;
    size_t next_component_key;

    [[nodiscard]] static auto make() -> ComponentAtlas {
        return ComponentAtlas{.component_to_key_map = std::flat_map<std::type_index, ComponentKey>(),
                              .component_datas = std::vector<ComponentData>(),
                              .next_component_key = 0};
    }

    auto destroy() const {}

    template <typename T> [[nodiscard]] auto get_or_create_key() -> ComponentKey {
        std::type_index type(typeid(T));
        auto key_it = component_to_key_map.find(type);
        if (key_it == component_to_key_map.end()) {
            if (next_component_key >= MAX_COMPONENT_COUNT) {
                assert(false);
                // TODO ERROR
                return (ComponentKey)-1;
            }

            ComponentKey key = next_component_key;
            ++next_component_key;

            component_to_key_map[type] = key;
            component_datas.push_back(ComponentData{.size = sizeof(T), .align = alignof(T)});

            return key;
        }

        return key_it->second;
    }

    template <typename T> [[nodiscard]] auto get_key() const -> std::optional<ComponentKey> {
        std::type_index type(typeid(T));
        auto key_it = component_to_key_map.find(type);
        if (key_it == component_to_key_map.end()) { return {}; }

        return key_it->second;
    }
    [[nodiscard]] auto get_key(std::type_index type) const -> std::optional<ComponentKey> {
        auto key_it = component_to_key_map.find(type);
        if (key_it == component_to_key_map.end()) { return {}; }

        return key_it->second;
    }

    template <typename T> [[nodiscard]] auto existed_type() const -> bool {
        return component_to_key_map.contains(std::type_index(typeid(T)));
    }
    [[nodiscard]] auto existed_type(std::type_index component) -> bool { return component_to_key_map.contains(component); }

    [[nodiscard]] auto get(ComponentKey key) -> ComponentData {
        assert(key < component_datas.size());
        return component_datas[key];
    }

    [[nodiscard]] auto has(ComponentKey key) -> bool { return key < component_datas.size(); }
};

} // namespace cactus
