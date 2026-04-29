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
    SignatureAtlasKey signature_key;
    size_t archetype_row_index;
};

export struct World {
    SlotMap<EntityData> entities_data;

    std::unordered_map<SignatureAtlasKey, ArchetypeAtlasKey> signature_to_archetype_key_map;

    ComponentSizeAlignAtlas component_size_align_atlas;
    SignatureAtlas signature_atlas;
    ArchetypeAtlas archetype_atlas;

    [[nodiscard]] static auto make() -> World {
        World world = World{.entities_data = {},
                            .signature_to_archetype_key_map = {},
                            .component_size_align_atlas = ComponentSizeAlignAtlas::make(),
                            .signature_atlas = SignatureAtlas::make()};
        world.archetype_atlas = ArchetypeAtlas::make(&world.component_size_align_atlas, &world.signature_atlas);
        return world;
    }

    auto destroy() const {
        entities_data.destroy();
        component_size_align_atlas.destroy();
        signature_atlas.destroy();
        archetype_atlas.destroy();
    }

    [[nodiscard]] auto create() -> Entity {
        return this->entities_data.push(EntityData{.signature_key = EMPTY_SIGNATURE_KEY, .archetype_row_index = 0});
    }

    [[nodiscard]] auto has(const Entity &entity) const -> bool { return this->entities_data.has(entity); }

    template <typename T> [[nodiscard]] auto get_component(const Entity &entity) const -> std::optional<T> {
        if (!this->has(entity)) return {};
        if (!this->has_component<T>(entity)) return {};

        std::type_index component = std::type_index(typeid(T));

        const EntityData entity_data = entities_data.get(entity).value();

        const Signature entity_signature = this->signature_atlas.get(entity_data.signature_key);
        if (!entity_signature.has(component)) return {};

        assert(this->signature_to_archetype_key_map.contains(entity_data.signature_key));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_data.signature_key);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        return *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component);
    }
    template <typename T> [[nodiscard]] auto get_component_ptr(const Entity &entity) -> std::optional<T *> {
        if (!this->has(entity)) return {};
        if (!this->has_component<T>(entity)) return {};

        std::type_index component = std::type_index(typeid(T));

        const EntityData entity_data = entities_data.get(entity).value();

        const Signature entity_signature = this->signature_atlas.get(entity_data.signature_key);
        if (!entity_signature.has(component)) return {};

        assert(this->signature_to_archetype_key_map.contains(entity_data.signature_key));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_data.signature_key);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        return (T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component);
    }

    template <typename T> [[nodiscard]] auto has_component(const Entity &entity) const -> bool {
        if (!this->has(entity)) return false;

        std::type_index component = std::type_index(typeid(T));

        const EntityData entity_data = entities_data.get(entity).value();

        if (entity_data.signature_key == EMPTY_SIGNATURE_KEY) return false;

        const Signature entity_signature = this->signature_atlas.get(entity_data.signature_key);

        return entity_signature.has(component);
    }

    template <typename T> auto set_component(const Entity &entity, const T &val) -> bool {
        if (!this->has(entity)) return false;
        if (!this->has_component<T>(entity)) return false;

        std::type_index component = std::type_index(typeid(T));

        const EntityData entity_data = entities_data.get(entity).value();

        const Signature entity_signature = this->signature_atlas.get(entity_data.signature_key);
        if (!entity_signature.has(component)) return false;

        assert(this->signature_to_archetype_key_map.contains(entity_data.signature_key));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_data.signature_key);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component) = val;

        return true;
    }

    template <typename T> auto add_component(const Entity &entity, const T &val) -> bool {
        if (!this->has(entity)) return false;

        this->component_size_align_atlas.push<T>();

        std::type_index component = std::type_index(typeid(T));

        const EntityData entity_data = entities_data.get(entity).value();

        if (entity_data.signature_key == EMPTY_SIGNATURE_KEY) {
            SignatureAtlasKey new_signature_key =
                this->signature_atlas.get_key_by_add_component(entity_data.signature_key, component);
            const Signature new_signature = this->signature_atlas.get(new_signature_key);

            auto new_archetype_key_it = this->signature_to_archetype_key_map.find(new_signature_key);
            ArchetypeAtlasKey new_archetype_key = new_archetype_key_it != this->signature_to_archetype_key_map.end()
                                                      ? new_archetype_key_it->second
                                                      : this->archetype_atlas.new_archetype(new_signature_key);
            if (new_archetype_key_it == this->signature_to_archetype_key_map.end()) {
                this->signature_to_archetype_key_map[new_signature_key] = new_archetype_key;
            }

            size_t new_row_index = this->archetype_atlas.append(new_archetype_key);
            assert(new_row_index != -1);

            const Archetype new_archetype = this->archetype_atlas.get(new_archetype_key);
            *(T *)new_archetype.get_component_ptr(new_row_index, component) = val;

            entities_data.set(entity, EntityData{.signature_key = new_signature_key, .archetype_row_index = new_row_index});

            return true;
        }

        const Signature entity_signature = this->signature_atlas.get(entity_data.signature_key);

        assert(this->signature_to_archetype_key_map.contains(entity_data.signature_key));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_data.signature_key);

        if (entity_signature.has(component)) {
            const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
            *(T *)entity_archetype.get_component_ptr(entity_data.archetype_row_index, component) = val;

            return false;
        }

        SignatureAtlasKey new_signature_key =
            this->signature_atlas.get_key_by_add_component(entity_data.signature_key, component);
        const Signature new_signature = this->signature_atlas.get(new_signature_key);

        auto new_archetype_key_it = this->signature_to_archetype_key_map.find(new_signature_key);
        ArchetypeAtlasKey new_archetype_key = new_archetype_key_it != this->signature_to_archetype_key_map.end()
                                                  ? new_archetype_key_it->second
                                                  : this->archetype_atlas.new_archetype(new_signature_key);
        if (new_archetype_key_it == this->signature_to_archetype_key_map.end()) {
            this->signature_to_archetype_key_map[new_signature_key] = new_archetype_key;
        }

        size_t new_row_index = this->archetype_atlas.append(new_archetype_key);
        assert(new_row_index != -1);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        const Archetype new_archetype = this->archetype_atlas.get(new_archetype_key);

        this->handle_copy_row(entity_archetype, entity_data.archetype_row_index, new_archetype, new_row_index);

        *(T *)new_archetype.get_component_ptr(new_row_index, component) = val;

        this->archetype_atlas.remove(entity_archetype_key, entity_data.archetype_row_index);

        entities_data.set(entity, EntityData{.signature_key = new_signature_key, .archetype_row_index = new_row_index});

        return true;
    }

    template <typename T> auto remove_component(const Entity &entity) -> bool {
        if (!this->has(entity)) return false;
        std::type_index component = std::type_index(typeid(T));

        const EntityData entity_data = entities_data.get(entity).value();

        const Signature entity_signature = this->signature_atlas.get(entity_data.signature_key);

        assert(this->signature_to_archetype_key_map.contains(entity_data.signature_key));
        ArchetypeAtlasKey entity_archetype_key = this->signature_to_archetype_key_map.at(entity_data.signature_key);

        if (!entity_signature.has(component)) return false;

        SignatureAtlasKey new_signature_key =
            this->signature_atlas.get_key_by_remove_component(entity_data.signature_key, component);
        const Signature new_signature = this->signature_atlas.get(new_signature_key);

        if (new_signature_key == EMPTY_SIGNATURE_KEY) {
            this->archetype_atlas.remove(entity_archetype_key, entity_data.archetype_row_index);

            entities_data.set(entity, EntityData{.signature_key = new_signature_key, .archetype_row_index = 0});

            return true;
        }

        auto new_archetype_key_it = this->signature_to_archetype_key_map.find(new_signature_key);
        ArchetypeAtlasKey new_archetype_key = new_archetype_key_it != this->signature_to_archetype_key_map.end()
                                                  ? new_archetype_key_it->second
                                                  : this->archetype_atlas.new_archetype(new_signature_key);
        if (new_archetype_key_it == this->signature_to_archetype_key_map.end()) {
            this->signature_to_archetype_key_map[new_signature_key] = new_archetype_key;
        }

        size_t new_row_index = this->archetype_atlas.append(new_archetype_key);
        assert(new_row_index != -1);

        const Archetype entity_archetype = this->archetype_atlas.get(entity_archetype_key);
        const Archetype new_archetype = this->archetype_atlas.get(new_archetype_key);

        this->handle_copy_row(entity_archetype, entity_data.archetype_row_index, new_archetype, new_row_index);

        this->archetype_atlas.remove(entity_archetype_key, entity_data.archetype_row_index);

        entities_data.set(entity, EntityData{.signature_key = new_signature_key, .archetype_row_index = new_row_index});

        return true;
    }

private:
    auto handle_copy_row(const Archetype &from_archetype, size_t from_row_index, const Archetype &to_archetype,
                         size_t to_row_index) -> void {
        const Signature from_signature = this->signature_atlas.get(from_archetype.signature_key);
        const Signature to_signature = this->signature_atlas.get(to_archetype.signature_key);

        char *from_row_ptr = (char *)from_archetype.get_row_ptr(from_row_index);
        char *to_row_ptr = (char *)to_archetype.get_row_ptr(to_row_index);

        size_t from_row_offset = 0, to_row_offset = 0;

        size_t i = 0, j = 0;
        while (i < from_signature.len && j < to_signature.len) {
            std::type_index from_t = from_signature.get(i).value();
            std::type_index to_t = to_signature.get(j).value();

            ComponentSizeAlignData from_t_data = this->component_size_align_atlas.get_size_align(from_t);
            ComponentSizeAlignData to_t_data = this->component_size_align_atlas.get_size_align(to_t);

            if (from_t == to_t) {
                from_row_offset = align_up(from_row_offset, from_t_data.align);
                to_row_offset = align_up(to_row_offset, to_t_data.align);

                std::memcpy(to_row_ptr + to_row_offset, from_row_ptr + from_row_offset, from_t_data.size);

                from_row_offset += from_t_data.size;
                to_row_offset += to_t_data.size;
                ++i;
                ++j;
            } else if (from_t < to_t) {
                from_row_offset = align_up(from_row_offset, from_t_data.align) + from_t_data.size;
                ++i;
            } else {
                to_row_offset = align_up(to_row_offset, to_t_data.align) + to_t_data.size;
                ++j;
            }
        }
    }
};

} // namespace cactus
