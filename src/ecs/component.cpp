module;

#include <cstddef>
#include <cstdlib>
#include <tuplet/tuple.hpp>
#include <typeinfo>

export module Ecs:Component;

import :Common;

namespace cactus::ecs {

export using ComponentID = size_t;

export template <typename T>
[[nodiscard]] auto component_id() -> ComponentID {
    return typeid(T).hash_code();
}

export struct ComponentAtlas {
    bstu::unordered_flat_map<ComponentID, tuplet::tuple<size_t, size_t>> component_size_map;

    [[nodiscard]] auto contains(ComponentID id) const -> bool {
        return component_size_map.contains(id);
    }

    auto get_component_size(ComponentID id) const -> size_t {
        auto iter = component_size_map.find(id);
        if (iter == component_size_map.end()) return (size_t)-1;
        return tuplet::get<0>(iter->second);
    }
    auto get_component_alignment(ComponentID id) const -> size_t {
        auto iter = component_size_map.find(id);
        if (iter == component_size_map.end()) return (size_t)-1;
        return tuplet::get<1>(iter->second);
    }

    template <typename T>
    auto register_component() {
        auto id = component_id<T>();
        auto iter = component_size_map.find(id);
        if (iter != component_size_map.end()) return;
        component_size_map.emplace(id, {sizeof(T), alignof(T)});
    }
};

} // namespace cactus::ecs
