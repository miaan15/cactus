module;

#include <bitset>
#include <boost/container/vector.hpp>
#include <cstddef>
#include <cstdint>
#include <stack>
#include <type_traits>
#include <unordered_map>

export module ECS;

import SlotMap;

namespace bstc = boost::container;

namespace cactus::ecs {

export using Entity = uint64_t;

template <typename... Ts>
struct is_unique;

template <typename T>
struct is_unique<T> : std::true_type {};

template <typename T, typename... Us>
struct is_unique<T, Us...> {
    static constexpr bool value = (!std::is_same_v<T, Us> && ...) && is_unique<Us...>::value;
};

template <typename... Ts>
constexpr bool is_unique_v = is_unique<Ts...>::value;

export template <typename... Ts>
    requires is_unique_v<Ts...> && (sizeof...(Ts) <= 64)
struct SmallWorld {
    using ArchetypeBitset = std::bitset<64>;

    struct EntityData {
        ArchetypeBitset archetype;
    };

    struct EntityDataMap {
        bstc::vector<std::variant<EntityData, size_t>> data;


    };

    std::stack<Entity, bstc::vector<Entity>> next_entity_id{{0}};

    std::unordered_map<ArchetypeBitset, void *> archetype_map;

    [[nodiscard]] auto create_entity() -> Entity {
        auto entity = next_entity_id.top();
        next_entity_id.pop();
        if (next_entity_id.empty()) next_entity_id.push(entity + 1);

        // TODO
    }

    template <typename T, typename... Args>
    auto emplace(Entity entity, Args... args) {
        T component{std::forward<Args>(args)...};
    }

    template <typename T>
    static constexpr size_t component_index() {
        size_t index = 0;
        bool found = false;

        ((std::is_same_v<T, Ts> ? (found = true) : (index += !found)), ...);

        return found ? index : -1;
    }
};

export template <typename... Ts>
struct Archetype {
    SlotMap<std::tuple<Ts...>> data;

    template <typename T>
    static constexpr size_t component_offset() {
        size_t offset = 0;
        bool found = false;

        ((std::is_same_v<T, Ts> ? (found = true) : (offset += !found ? sizeof(Ts) : 0)), ...);

        return found ? offset : -1;
    }
};

} // namespace cactus::ecs
