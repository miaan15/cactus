module;

#include <cassert>

export module cactus.core.ecs:world;

export import :entity;
export import :component;
export import :signature;
export import :archetype;

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

    std::unordered_map<SignatureAtlasKey, ArchetypeAtlasKey> signature_to_archetype_key_map;

    ComponentAtlas component_atlas;
    SignatureAtlas signature_atlas;
    ArchetypeAtlas archetype_atlas;

    [[nodiscard]] static auto make() -> World {
        World world = World{.entities_data = SlotMap<EntityData>::make(),
                            .signature_to_archetype_key_map = std::unordered_map<SignatureAtlasKey, ArchetypeAtlasKey>(),
                            .component_atlas = ComponentAtlas::make(),
                            .signature_atlas = SignatureAtlas::make()};
        world.archetype_atlas = ArchetypeAtlas::make(&world.component_atlas, &world.signature_atlas);
        return world;
    }

    auto destroy() const {
        entities_data.destroy();
        component_atlas.destroy();
        signature_atlas.destroy();
        archetype_atlas.destroy();
    }

    [[nodiscard]] auto create() -> Entity {
        return entities_data.push(EntityData{.signature_key = EMPTY_SIGNATURE_KEY, .archetype_row_index = 0});
    }

    [[nodiscard]] auto has(const Entity &entity) const -> bool { return entities_data.has(entity); }

    template <typename T> [[nodiscard]] auto get_component(const Entity &entity) const -> std::optional<T> {
        if (!has(entity)) return {};
        if (!has_component<T>(entity)) return {};

        std::optional<ComponentKey> component_opt = component_atlas.get_key<T>();
        if (!component_opt.has_value()) return {};
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();

        if (!signature_atlas.signature_test(entity_data.signature_key, component)) return {};

        assert(signature_to_archetype_key_map.contains(entity_data.signature_key));
        ArchetypeAtlasKey entity_archetype_key = signature_to_archetype_key_map.at(entity_data.signature_key);

        const Archetype entity_archetype = archetype_atlas.get(entity_archetype_key);
        return *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component);
    }
    template <typename T> [[nodiscard]] auto get_component_ptr(const Entity &entity) -> std::optional<T *> {
        if (!has(entity)) return {};
        if (!has_component<T>(entity)) return {};

        std::optional<ComponentKey> component_opt = component_atlas.get_key<T>();
        if (!component_opt.has_value()) return {};
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();

        const Signature entity_signature = signature_atlas.get(entity_data.signature_key);
        if (!entity_signature.test(component)) return {};

        assert(signature_to_archetype_key_map.contains(entity_data.signature_key));
        ArchetypeAtlasKey entity_archetype_key = signature_to_archetype_key_map.at(entity_data.signature_key);

        const Archetype entity_archetype = archetype_atlas.get(entity_archetype_key);
        return (T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component);
    }

    template <typename T> [[nodiscard]] auto has_component(const Entity &entity) const -> bool {
        if (!has(entity)) return false;

        std::optional<ComponentKey> component_opt = component_atlas.get_key<T>();
        if (!component_opt.has_value()) return false;
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();

        return signature_atlas.signature_test(entity_data.signature_key, component);
    }

    template <typename T> auto set_component(const Entity &entity, const T &val) -> bool {
        if (!has(entity)) return false;
        if (!has_component<T>(entity)) return false;

        std::optional<ComponentKey> component_opt = component_atlas.get_key<T>();
        if (!component_opt.has_value()) return false;
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();

        const Signature entity_signature = signature_atlas.get(entity_data.signature_key);
        if (!entity_signature.test(component)) return false;

        assert(signature_to_archetype_key_map.contains(entity_data.signature_key));
        ArchetypeAtlasKey entity_archetype_key = signature_to_archetype_key_map.at(entity_data.signature_key);

        const Archetype entity_archetype = archetype_atlas.get(entity_archetype_key);
        *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component) = val;

        return true;
    }

    template <typename T> auto add_component(const Entity &entity, const T &val) -> bool {
        if (!has(entity)) return false;

        const ComponentKey component = component_atlas.get_or_create_key<T>();

        const EntityData entity_data = entities_data.get(entity).value();
        const SignatureAtlasKey entity_signature_key = entity_data.signature_key;

        const Signature entity_signature = signature_atlas.get(entity_signature_key);

        if (!entity_signature.test(component)) {
            SignatureAtlasKey new_signature_key = signature_atlas.get_or_create_by_add(entity_signature_key, component);
            const Signature new_signature = signature_atlas.get(new_signature_key);

            auto new_archetype_key_it = signature_to_archetype_key_map.find(new_signature_key);
            ArchetypeAtlasKey new_archetype_key = new_archetype_key_it != signature_to_archetype_key_map.end()
                                                      ? new_archetype_key_it->second
                                                      : archetype_atlas.archetype_make(new_signature_key);
            if (new_archetype_key_it == signature_to_archetype_key_map.end()) {
                signature_to_archetype_key_map[new_signature_key] = new_archetype_key;
            }

            size_t new_row_index = archetype_atlas.archetype_append(new_archetype_key);

            Archetype new_archetype = archetype_atlas.get(new_archetype_key);
            *(T *)new_archetype.get_component_ptr(new_row_index, component) = val;

            if (!entity_signature.none()) {
                ArchetypeAtlasKey entity_archetype_key = signature_to_archetype_key_map.at(entity_signature_key);
                Archetype entity_archetype = archetype_atlas.get(entity_archetype_key);

                Archetype *new_archetype_ptr = archetype_atlas.get_ptr(new_archetype_key);
                Archetype *entity_archetype_ptr = archetype_atlas.get_ptr(entity_archetype_key);
                handle_copy_row(*entity_archetype_ptr, entity_data.archetype_row_index, new_archetype_ptr, new_row_index);
                archetype_atlas.archetype_remove(entity_archetype_key, entity_data.archetype_row_index);
            }

            entities_data.set(entity, EntityData{.signature_key = new_signature_key, .archetype_row_index = new_row_index});

            return true;
        }

        ArchetypeAtlasKey entity_archetype_key = signature_to_archetype_key_map.at(entity_signature_key);
        Archetype entity_archetype = archetype_atlas.get(entity_archetype_key);
        *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component) = val;

        return false;
    }

    template <typename T> auto remove_component(const Entity &entity) -> bool {
        if (!has(entity)) return false;

        std::optional<ComponentKey> component_opt = component_atlas.get_key<T>();
        if (!component_opt.has_value()) return false;
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = entities_data.get(entity).value();
        const SignatureAtlasKey entity_signature_key = entity_data.signature_key;

        const Signature entity_signature = signature_atlas.get(entity_signature_key);

        if (!entity_signature.test(component)) return false;

        SignatureAtlasKey new_signature_key = signature_atlas.get_or_create_by_remove(entity_signature_key, component);
        const Signature new_signature = signature_atlas.get(new_signature_key);

        ArchetypeAtlasKey entity_archetype_key = signature_to_archetype_key_map.at(entity_signature_key);

        if (new_signature.none()) {
            archetype_atlas.archetype_remove(entity_archetype_key, entity_data.archetype_row_index);

            entities_data.set(entity, EntityData{.signature_key = EMPTY_SIGNATURE_KEY, .archetype_row_index = 0});

            return true;
        }

        auto new_archetype_key_it = signature_to_archetype_key_map.find(new_signature_key);
        ArchetypeAtlasKey new_archetype_key = new_archetype_key_it != signature_to_archetype_key_map.end()
                                                  ? new_archetype_key_it->second
                                                  : archetype_atlas.archetype_make(new_signature_key);
        if (new_archetype_key_it == signature_to_archetype_key_map.end()) {
            signature_to_archetype_key_map[new_signature_key] = new_archetype_key;
        }

        size_t new_row_index = archetype_atlas.archetype_append(new_archetype_key);

        const Archetype entity_archetype = archetype_atlas.get(entity_archetype_key);
        Archetype *new_archetype = archetype_atlas.get_ptr(new_archetype_key);

        Archetype *entity_archetype_ptr = archetype_atlas.get_ptr(entity_archetype_key);
        handle_copy_row(*entity_archetype_ptr, entity_data.archetype_row_index, new_archetype, new_row_index);

        archetype_atlas.archetype_remove(entity_archetype_key, entity_data.archetype_row_index);

        entities_data.set(entity, EntityData{.signature_key = new_signature_key, .archetype_row_index = new_row_index});

        return true;
    }

private:
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

            ComponentData from_c_data = component_atlas.get(from_component);
            ComponentData to_c_data = component_atlas.get(to_component);

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
