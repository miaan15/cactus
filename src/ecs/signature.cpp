module;

#include <cstddef>
#include <cassert>
#include <cstring>
#include <cstdlib>

export module Ecs:Signature;

import :Common;
import :Component;

namespace cactus::ecs {

export using SignatureID = size_t;

export constexpr SignatureID EMPTY_SIGNATURE_ID = (SignatureID)-1;

export struct SignatureAtlas {
    struct SignatureContainer {
        ComponentID *ptr;
        size_t size;
    };

    struct SignatureGraphTransition {
        SignatureID from;
        ComponentID component;

        auto operator<=>(const SignatureGraphTransition &) const = default;

        struct Hasher {
            auto operator()(const SignatureGraphTransition &t) const -> size_t {
                size_t seed = 0;
                boost::hash_combine(seed, t.from);
                boost::hash_combine(seed, t.component);
                return seed;
            }
        };
    };

    bstc::vector<SignatureContainer> signature_containers;
    bstu::unordered_flat_map<SignatureGraphTransition, SignatureID,
                             SignatureGraphTransition::Hasher>
        signature_add_component_graph;
    bstu::unordered_flat_map<SignatureGraphTransition, SignatureID,
                             SignatureGraphTransition::Hasher>
        signature_remove_component_graph;

    SignatureAtlas() = default;
    SignatureAtlas(const SignatureAtlas &) = delete;
    SignatureAtlas &operator=(const SignatureAtlas &) = delete;
    SignatureAtlas(SignatureAtlas &&) noexcept = default;
    SignatureAtlas &operator=(SignatureAtlas &&) noexcept = default;
    ~SignatureAtlas() {
        for (auto &c : signature_containers) {
            free(c.ptr);
        }
    }

    [[nodiscard]] auto get_container(SignatureID id) -> SignatureContainer * {
        assert(id < signature_containers.size());
        return &signature_containers[id];
    }
    [[nodiscard]] auto get_container(SignatureID id) const -> const SignatureContainer * {
        assert(id < signature_containers.size());
        return &signature_containers[id];
    }

    [[nodiscard]] auto component_index(SignatureID sid, ComponentID cid) const -> size_t {
        assert(sid < signature_containers.size());
        assert(sid != EMPTY_SIGNATURE_ID);
        for (size_t i = 0; i < signature_containers[sid].size; ++i) {
            if (signature_containers[sid].ptr[i] == cid) return i;
        }
        assert(false);
    }

    [[nodiscard]] auto contains_component(SignatureID sid, ComponentID cid) const -> bool {
        assert(sid < signature_containers.size());
        if (sid == EMPTY_SIGNATURE_ID) return false;
        for (size_t i = 0; i < signature_containers[sid].size; ++i) {
            if (signature_containers[sid].ptr[i] == cid) return true;
        }
        return false;
    }

    [[nodiscard]] auto create_or_get_signature_by_add_component(SignatureID existed_id,
                                                                ComponentID cid) -> SignatureID {
        assert(existed_id < signature_containers.size());
        assert(!contains_component(existed_id, cid));

        auto graph_iter = signature_add_component_graph.find({existed_id, cid});
        if (graph_iter != signature_add_component_graph.end()) {
            return graph_iter->second;
        }

        if (existed_id == EMPTY_SIGNATURE_ID) {
            signature_containers.emplace_back((ComponentID *)malloc(1 * sizeof(ComponentID)), 1);
            signature_containers.back().ptr[0] = cid;

            signature_add_component_graph.emplace(SignatureGraphTransition{existed_id, cid},
                                                  signature_containers.size() - 1);
            signature_remove_component_graph.emplace(
                SignatureGraphTransition{signature_containers.size() - 1, cid}, existed_id);
            return signature_containers.size() - 1;
        }

        const auto &existed_signature_container = signature_containers.at(existed_id);

        signature_containers.emplace_back(
            (ComponentID *)malloc((existed_signature_container.size + 1) * sizeof(ComponentID)),
            existed_signature_container.size + 1);

        memcpy(signature_containers.back().ptr, existed_signature_container.ptr,
               existed_signature_container.size * sizeof(ComponentID));
        signature_containers.back().ptr[existed_signature_container.size] = cid;

        signature_add_component_graph.emplace(SignatureGraphTransition{existed_id, cid},
                                              signature_containers.size() - 1);
        signature_remove_component_graph.emplace(
            SignatureGraphTransition{signature_containers.size() - 1, cid}, existed_id);

        return signature_containers.size() - 1;
    }

    [[nodiscard]] auto create_or_get_signature_by_remove_component(SignatureID existed_id,
                                                                   ComponentID cid) -> SignatureID {
        assert(existed_id < signature_containers.size());
        assert(contains_component(existed_id, cid));

        auto graph_iter = signature_remove_component_graph.find({existed_id, cid});
        if (graph_iter != signature_remove_component_graph.end()) {
            return graph_iter->second;
        }

        const auto &existed_signature_container = signature_containers.at(existed_id);

        signature_containers.emplace_back(
            (ComponentID *)malloc((existed_signature_container.size - 1) * sizeof(ComponentID)),
            existed_signature_container.size - 1);

        assert(signature_containers.back().size != 0);

        auto component_offset = (size_t)-1;
        for (int i = 0; i < existed_signature_container.size; ++i) {
            if (existed_signature_container.ptr[i] == cid) component_offset = i;
        }
        memcpy(signature_containers.back().ptr, existed_signature_container.ptr,
               component_offset * sizeof(ComponentID));
        memcpy(signature_containers.back().ptr + component_offset,
               existed_signature_container.ptr + (component_offset + 1),
               component_offset * sizeof(ComponentID));

        signature_remove_component_graph.emplace(SignatureGraphTransition{existed_id, cid},
                                                 signature_containers.size() - 1);
        signature_add_component_graph.emplace(
            SignatureGraphTransition{signature_containers.size() - 1, cid}, existed_id);

        return signature_containers.size() - 1;
    }
};

} // namespace cactus::ecs
