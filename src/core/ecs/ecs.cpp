module;

#include <cassert>

export module cactus.core.ecs;

// World is the ground that holds entities and datas (Archetypes)
// Entity is just a key to access to Component data
// Component is plain data of a type T
// Signature is a list of std::type_index (type of Component)
// Archetype hold all actual Component data of a single Signature
// .
// Component is represented by std::type_index
// Archetype structured like a table
// Signature is a blueprint of what component Entity/Archetype has

export import :entity;
export import :component;
export import :signature;
export import :archetype;

import std;
import cactus.core.strat;

using size_t = std::size_t;

namespace cactus {

export struct EntityData {
    Signature signature;
    size_t archetype_row_index;
};

export struct World {
    SlotMap<EntityData> entities_data;

    std::unordered_map<Signature, ArchetypeAtlasKey> signature_to_archetype_key_map;

    ComponentAtlas component_atlas;
    ArchetypeAtlas archetype_atlas;

    [[nodiscard]] static auto make() -> World {
        World world = World{.entities_data = SlotMap<EntityData>::make(),
                            .signature_to_archetype_key_map = std::unordered_map<Signature, ArchetypeAtlasKey>(),
                            .component_atlas = ComponentAtlas::make()};
        world.archetype_atlas = ArchetypeAtlas::make(&world.component_atlas);
        return world;
    }

    auto destroy() const {
        this->entities_data.destroy();
        this->component_atlas.destroy();
        this->archetype_atlas.destroy();
    }

    [[nodiscard]] auto create() -> Entity {
        return this->entities_data.push(EntityData{.signature = Signature(), .archetype_row_index = 0});
    }

    [[nodiscard]] auto has(const Entity &entity) const -> bool { return this->entities_data.has(entity); }

    template <typename T> [[nodiscard]] auto get_component(const Entity &entity) const -> std::optional<T> {
        if (!this->has(entity)) return {};
        if (!this->has_component<T>(entity)) return {};

        std::optional<ComponentKey> component_opt = this->component_atlas.get_key_const<T>();
        if (!component_opt.has_value()) return {};
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = this->entities_data.get(entity).value();

        const Signature entity_signature = entity_data.signature;
        if (!entity_signature.test(component)) return {};

        assert(this->signature_to_archetype_key_map.contains(entity_signature));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_signature);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        return *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component);
    }
    template <typename T> [[nodiscard]] auto get_component_ptr(const Entity &entity) -> std::optional<T *> {
        if (!this->has(entity)) return {};
        if (!this->has_component<T>(entity)) return {};

        const ComponentKey component = this->component_atlas.get_key<T>();

        const EntityData entity_data = this->entities_data.get(entity).value();

        const Signature entity_signature = entity_data.signature;
        if (!entity_signature.test(component)) return {};

        assert(this->signature_to_archetype_key_map.contains(entity_signature));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_signature);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        return (T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component);
    }

    template <typename T> [[nodiscard]] auto has_component(const Entity &entity) const -> bool {
        if (!this->has(entity)) return false;

        std::optional<ComponentKey> component_opt = this->component_atlas.get_key_const<T>();
        if (!component_opt.has_value()) return false;
        const ComponentKey component = component_opt.value();

        const EntityData entity_data = this->entities_data.get(entity).value();

        return entity_data.signature.test(component);
    }

    template <typename T> auto set_component(const Entity &entity, const T &val) -> bool {
        if (!this->has(entity)) return false;
        if (!this->has_component<T>(entity)) return false;

        const ComponentKey component = this->component_atlas.get_key<T>();

        const EntityData entity_data = this->entities_data.get(entity).value();

        const Signature entity_signature = entity_data.signature;
        if (!entity_signature.test(component)) return false;

        assert(this->signature_to_archetype_key_map.contains(entity_signature));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_signature);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component) = val;

        return true;
    }

    template <typename T> auto add_component(const Entity &entity, const T &val) -> bool {
        if (!this->has(entity)) return false;

        const ComponentKey component = this->component_atlas.get_key<T>();

        const EntityData entity_data = this->entities_data.get(entity).value();

        const Signature entity_signature = entity_data.signature;

        if (entity_signature.none()) {
            Signature new_signature = entity_signature;
            new_signature.set(component);

            auto new_archetype_key_it = this->signature_to_archetype_key_map.find(new_signature);
            ArchetypeAtlasKey new_archetype_key = new_archetype_key_it != this->signature_to_archetype_key_map.end()
                                                      ? new_archetype_key_it->second
                                                      : this->archetype_atlas.new_archetype(new_signature);
            if (new_archetype_key_it == this->signature_to_archetype_key_map.end()) {
                this->signature_to_archetype_key_map[new_signature] = new_archetype_key;
            }

            size_t new_row_index = this->archetype_atlas.append(new_archetype_key);
            assert(new_row_index != -1);

            const Archetype new_archetype = this->archetype_atlas.get(new_archetype_key);
            *(T *)new_archetype.get_component_ptr(new_row_index, component) = val;

            this->entities_data.set(entity, EntityData{.signature = new_signature, .archetype_row_index = new_row_index});

            return true;
        }

        assert(this->signature_to_archetype_key_map.contains(entity_signature));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_signature);

        if (entity_signature.test(component)) {
            const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
            *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component) = val;

            return false;
        }

        Signature new_signature = entity_signature;
        new_signature.set(component);

        auto new_archetype_key_it = this->signature_to_archetype_key_map.find(new_signature);
        ArchetypeAtlasKey new_archetype_key = new_archetype_key_it != this->signature_to_archetype_key_map.end()
                                                  ? new_archetype_key_it->second
                                                  : this->archetype_atlas.new_archetype(new_signature);
        if (new_archetype_key_it == this->signature_to_archetype_key_map.end()) {
            this->signature_to_archetype_key_map[new_signature] = new_archetype_key;
        }

        size_t new_row_index = this->archetype_atlas.append(new_archetype_key);
        assert(new_row_index != -1);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        Archetype new_archetype = this->archetype_atlas.get(new_archetype_key);

        this->handle_copy_row(entity_archetype, entity_data.archetype_row_index, &new_archetype, new_row_index);

        *(T *)new_archetype.get_component_ptr(new_row_index, component) = val;

        this->archetype_atlas.remove(entity_archetype_key, entity_data.archetype_row_index);

        this->entities_data.set(entity, EntityData{.signature = new_signature, .archetype_row_index = new_row_index});

        return true;
    }

    template <typename T> auto remove_component(const Entity &entity) -> bool {
        if (!this->has(entity)) return false;
        const ComponentKey component = this->component_atlas.get_key<T>();

        const EntityData entity_data = this->entities_data.get(entity).value();

        const Signature entity_signature = entity_data.signature;

        assert(this->signature_to_archetype_key_map.contains(entity_signature));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_signature);

        if (!entity_signature.test(component)) return false;

        Signature new_signature = entity_signature;
        new_signature.reset(component);

        if (new_signature.none()) {
            this->archetype_atlas.remove(entity_archetype_key, entity_data.archetype_row_index);

            this->entities_data.set(entity, EntityData{.signature = new_signature, .archetype_row_index = 0});

            return true;
        }

        auto new_archetype_key_it = this->signature_to_archetype_key_map.find(new_signature);
        ArchetypeAtlasKey new_archetype_key = new_archetype_key_it != this->signature_to_archetype_key_map.end()
                                                  ? new_archetype_key_it->second
                                                  : this->archetype_atlas.new_archetype(new_signature);
        if (new_archetype_key_it == this->signature_to_archetype_key_map.end()) {
            this->signature_to_archetype_key_map[new_signature] = new_archetype_key;
        }

        size_t new_row_index = this->archetype_atlas.append(new_archetype_key);
        assert(new_row_index != -1);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        Archetype new_archetype = this->archetype_atlas.get(new_archetype_key);

        this->handle_copy_row(entity_archetype, entity_data.archetype_row_index, &new_archetype, new_row_index);

        this->archetype_atlas.remove(entity_archetype_key, entity_data.archetype_row_index);

        this->entities_data.set(entity, EntityData{.signature = new_signature, .archetype_row_index = new_row_index});

        return true;
    }

private:
    auto handle_copy_row(const Archetype &from_archetype, size_t from_row_index, Archetype *to_archetype, size_t to_row_index)
        -> void {
        const SignatureView from_signature_view = SignatureView(from_archetype.signature);
        const SignatureView to_signature_view = SignatureView(to_archetype->signature);

        char *from_row_ptr = (char *)from_archetype.get_row_ptr(from_row_index);
        char *to_row_ptr = (char *)to_archetype->get_row_ptr(to_row_index);

        size_t from_row_offset = 0, to_row_offset = 0;

        auto from_signature_it = from_signature_view.begin();
        auto to_signature_it = to_signature_view.begin();
        while (from_signature_it != from_signature_view.end() && to_signature_it != to_signature_view.end()) {
            const ComponentKey from_component = *from_signature_it;
            const ComponentKey to_component = *to_signature_it;

            ComponentData from_c_data = this->component_atlas.get(from_component);
            ComponentData to_c_data = this->component_atlas.get(to_component);

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
