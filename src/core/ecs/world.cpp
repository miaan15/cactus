module;

#include <cassert>

export module cactus.core.ecs:world;

import :entity;
import :component;
import :signature;
import :archetype;

import std;
import cactus.core.strat;

using size_t = std::size_t;

namespace cactus {

export struct World {
    struct EntityData {
        SignatureAtlasKey signature_key;
        size_t archetype_row_index;
    };

    SlotMap<EntityData> entities_data;

    std::unordered_map<SignatureAtlasKey, size_t> signature_to_archetype_key_map;

    ComponentRegistry component_registry;
    SignatureAtlas signature_atlas;
    std::vector<Archetype> archetypes;

    [[nodiscard]] static auto make() -> World {
        World world = World{.entities_data = SlotMap<EntityData>::make(),
                            .signature_to_archetype_key_map = std::unordered_map<SignatureAtlasKey, size_t>(),
                            .component_registry = ComponentRegistry::make(),
                            .signature_atlas = SignatureAtlas::make(),
                            .archetypes = std::vector<Archetype>()};
        return world;
    }

    auto destroy() const {
        entities_data.destroy();
        component_registry.destroy();
        signature_atlas.destroy();
        for (const auto &a : archetypes) a.destroy();
    }

    [[nodiscard]] auto create() -> Entity {
        return entities_data.push(EntityData{.signature_key = EMPTY_SIGNATURE_KEY, .archetype_row_index = 0});
    }

    [[nodiscard]] auto has(const Entity &entity) const -> bool { return entities_data.has(entity); }

    template <typename T> [[nodiscard]] auto get_component(const Entity &entity) const -> std::optional<T> {
        if (!has(entity)) return {};
        if (!has_component<T>(entity)) return {};

        std::optional<ComponentKey> component_opt = component_registry.get_key<T>();
        if (!component_opt.has_value()) return {};
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();

        if (!signature_atlas.signature_test(entity_data.signature_key, component)) return {};

        assert(signature_to_archetype_key_map.contains(entity_data.signature_key));
        size_t entity_archetype_key = signature_to_archetype_key_map.at(entity_data.signature_key);
        assert(entity_archetype_key < archetypes.size());

        const Archetype entity_archetype = archetypes[entity_archetype_key];
        return *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component);
    }
    template <typename T> [[nodiscard]] auto get_component_ptr(const Entity &entity) -> std::optional<T *> {
        if (!has(entity)) return {};
        if (!has_component<T>(entity)) return {};

        std::optional<ComponentKey> component_opt = component_registry.get_key<T>();
        if (!component_opt.has_value()) return {};
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();

        if (!signature_atlas.signature_test(entity_data.signature_key, component)) return {};

        assert(signature_to_archetype_key_map.contains(entity_data.signature_key));
        size_t entity_archetype_key = signature_to_archetype_key_map.at(entity_data.signature_key);
        assert(entity_archetype_key < archetypes.size());

        const Archetype entity_archetype = archetypes[entity_archetype_key];
        return (T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component);
    }

    template <typename T> [[nodiscard]] auto has_component(const Entity &entity) const -> bool {
        if (!has(entity)) return false;

        std::optional<ComponentKey> component_opt = component_registry.get_key<T>();
        if (!component_opt.has_value()) return false;
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();

        return signature_atlas.signature_test(entity_data.signature_key, component);
    }

    template <typename T> auto set_component(const Entity &entity, const T &val) -> bool {
        if (!has(entity)) return false;
        if (!has_component<T>(entity)) return false;

        std::optional<ComponentKey> component_opt = component_registry.get_key<T>();
        if (!component_opt.has_value()) return false;
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();

        if (!signature_atlas.signature_test(entity_data.signature_key, component)) return false;

        assert(signature_to_archetype_key_map.contains(entity_data.signature_key));
        size_t entity_archetype_key = signature_to_archetype_key_map.at(entity_data.signature_key);
        assert(entity_archetype_key < archetypes.size());

        const Archetype entity_archetype = archetypes[entity_archetype_key];
        *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component) = val;

        return true;
    }

    template <typename T> auto add_component(const Entity &entity, const T &val) -> bool {
        if (!has(entity)) return false;

        const ComponentKey component = component_registry.get_or_create_key<T>();

        const EntityData entity_data = entities_data.get(entity).value();
        const SignatureAtlasKey entity_signature_key = entity_data.signature_key;

        if (!signature_atlas.signature_test(entity_signature_key, component)) {
            SignatureAtlasKey new_signature_key = signature_atlas.get_or_create_by_add(entity_signature_key, component);

            auto new_archetype_key_it = signature_to_archetype_key_map.find(new_signature_key);
            size_t new_archetype_key = new_archetype_key_it != signature_to_archetype_key_map.end()
                                           ? new_archetype_key_it->second
                                           : create_new_archetype(new_signature_key);
            if (new_archetype_key_it == signature_to_archetype_key_map.end()) {
                signature_to_archetype_key_map[new_signature_key] = new_archetype_key;
            }

            assert(new_archetype_key < archetypes.size());
            size_t new_row_index = archetypes[new_archetype_key].append(entity);

            Archetype new_archetype = archetypes[new_archetype_key];
            *(T *)new_archetype.get_component_ptr(new_row_index, component) = val;

            if (entity_signature_key != EMPTY_SIGNATURE_KEY) {
                size_t entity_archetype_key = signature_to_archetype_key_map.at(entity_signature_key);
                assert(entity_archetype_key < archetypes.size());

                Archetype entity_archetype = archetypes[entity_archetype_key];

                Archetype *new_archetype_ptr = &archetypes[new_archetype_key];
                Archetype *entity_archetype_ptr = &archetypes[entity_archetype_key];
                handle_copy_row(*entity_archetype_ptr, entity_data.archetype_row_index, new_archetype_ptr, new_row_index);

                assert(entity_archetype_key < archetypes.size());
                archetypes[entity_archetype_key].remove(entity_data.archetype_row_index);
            }

            entities_data.set(entity, EntityData{.signature_key = new_signature_key, .archetype_row_index = new_row_index});

            return true;
        }

        size_t entity_archetype_key = signature_to_archetype_key_map.at(entity_signature_key);
        assert(entity_archetype_key < archetypes.size());
        Archetype entity_archetype = archetypes[entity_archetype_key];
        *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component) = val;

        return false;
    }

    template <typename T> auto remove_component(const Entity &entity) -> bool {
        if (!has(entity)) return false;

        std::optional<ComponentKey> component_opt = component_registry.get_key<T>();
        if (!component_opt.has_value()) return false;
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();
        const SignatureAtlasKey entity_signature_key = entity_data.signature_key;

        if (!signature_atlas.signature_test(entity_signature_key, component)) return false;

        SignatureAtlasKey new_signature_key = signature_atlas.get_or_create_by_remove(entity_signature_key, component);

        size_t entity_archetype_key = signature_to_archetype_key_map.at(entity_signature_key);
        assert(entity_archetype_key < archetypes.size());

        if (new_signature_key == EMPTY_SIGNATURE_KEY) {
            archetypes[entity_archetype_key].remove(entity_data.archetype_row_index);

            entities_data.set(entity, EntityData{.signature_key = EMPTY_SIGNATURE_KEY, .archetype_row_index = 0});

            return true;
        }

        auto new_archetype_key_it = signature_to_archetype_key_map.find(new_signature_key);
        size_t new_archetype_key = new_archetype_key_it != signature_to_archetype_key_map.end()
                                       ? new_archetype_key_it->second
                                       : create_new_archetype(new_signature_key);
        if (new_archetype_key_it == signature_to_archetype_key_map.end()) {
            signature_to_archetype_key_map[new_signature_key] = new_archetype_key;
        }

        assert(new_archetype_key < archetypes.size());
        size_t new_row_index = archetypes[new_archetype_key].append(entity);

        const Archetype entity_archetype = archetypes[entity_archetype_key];
        Archetype *new_archetype = &archetypes[new_archetype_key];

        Archetype *entity_archetype_ptr = &archetypes[entity_archetype_key];
        handle_copy_row(*entity_archetype_ptr, entity_data.archetype_row_index, new_archetype, new_row_index);

        assert(entity_archetype_key < archetypes.size());
        archetypes[entity_archetype_key].remove(entity_data.archetype_row_index);

        entities_data.set(entity, EntityData{.signature_key = new_signature_key, .archetype_row_index = new_row_index});

        return true;
    }

private:
    auto create_new_archetype(SignatureAtlasKey signature_key) -> size_t {
        archetypes.push_back(Archetype::make(&component_registry, &signature_atlas, signature_key));
        return archetypes.size() - 1;
    }

    auto handle_copy_row(const Archetype &from_archetype, size_t from_row_index, Archetype *to_archetype, size_t to_row_index)
        -> void {
        const Signature from_signature = signature_atlas.get(from_archetype.signature_key);
        const Signature to_signature = signature_atlas.get(to_archetype->signature_key);

        const SignatureView from_signature_view = SignatureView(from_signature);
        const SignatureView to_signature_view = SignatureView(to_signature);

        char *from_row_ptr = (char *)from_archetype.get_row_ptr(from_row_index);
        char *to_row_ptr = (char *)to_archetype->get_row_ptr(to_row_index);

        size_t from_row_offset = 0, to_row_offset = 0;

        auto from_signature_it = from_signature_view.begin();
        auto to_signature_it = to_signature_view.begin();
        while (from_signature_it != from_signature_view.end() && to_signature_it != to_signature_view.end()) {
            const ComponentKey from_component = *from_signature_it;
            const ComponentKey to_component = *to_signature_it;

            ComponentData from_c_data = component_registry.get_component_data(from_component);
            ComponentData to_c_data = component_registry.get_component_data(to_component);

            if (from_component == to_component) {
                from_row_offset = align_up(from_row_offset, from_c_data.align);
                to_row_offset = align_up(to_row_offset, to_c_data.align);

                std::memcpy(to_row_ptr + to_row_offset, from_row_ptr + from_row_offset, from_c_data.size);

                from_row_offset += from_c_data.size;
                to_row_offset += to_c_data.size;
                ++from_signature_it;
                ++to_signature_it;
            } else if (from_component < to_component) {
                from_row_offset = align_up(from_row_offset, from_c_data.align) + from_c_data.size;
                ++from_signature_it;
            } else {
                to_row_offset = align_up(to_row_offset, to_c_data.align) + to_c_data.size;
                ++to_signature_it;
            }
        }
    }
};

} // namespace cactus
