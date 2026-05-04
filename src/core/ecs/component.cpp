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

    template <typename T> [[nodiscard]] auto get_key() -> ComponentKey {
        std::type_index type(typeid(T));
        auto key_it = this->component_to_key_map.find(type);
        if (key_it == this->component_to_key_map.end()) {
            if (this->next_component_key >= MAX_COMPONENT_COUNT) {
                assert(false);
                // TODO ERROR
                return (ComponentKey)-1;
            }

            this->component_datas.push_back(ComponentData{.size = sizeof(T), .align = alignof(T)});

            ++this->next_component_key;

            return this->next_component_key - 1;
        }

        return key_it->second;
    }

    template <typename T> [[nodiscard]] auto get_key_const() const -> std::optional<ComponentKey> {
        std::type_index type(typeid(T));
        auto key_it = this->component_to_key_map.find(type);
        if (key_it == this->component_to_key_map.end()) { return {}; }

        return key_it->second;
    }

    template <typename T> [[nodiscard]] auto has_type() -> bool {
        return this->component_to_key_map.contains(std::type_index(typeid(T)));
    }
    [[nodiscard]] auto has_type(std::type_index component) -> bool { return this->component_to_key_map.contains(component); }

    [[nodiscard]] auto get(ComponentKey key) -> ComponentData {
        assert(key < this->component_datas.size());
        return this->component_datas[key];
    }

    [[nodiscard]] auto has(ComponentKey key) -> bool { return key < this->component_datas.size(); }
};

} // namespace cactus
