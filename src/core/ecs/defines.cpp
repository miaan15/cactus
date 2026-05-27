module;

export module cactus.core.ecs:defines;

import cactus.common;
import cactus.core.strat;

namespace cactus {

export constexpr size_t MAX_WORLD_COMPONENTS_COUNT = 32;

export using Entity = SlotMapKey;

export using Signature = std::bitset<MAX_WORLD_COMPONENTS_COUNT>;
export struct SignatureHasher {
    auto operator()(const Signature &s) const -> size_t { return static_cast<size_t>(s.to_ullong()); }
};

export struct ComponentData {
    size_t size, align;
};

export template <typename... Ts>
    requires(std::is_trivially_copyable_v<Ts> && ...)
struct World;

} // namespace cactus
