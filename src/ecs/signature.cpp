module;

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

export module Ecs:Signature;

import :Common;
import :Component;

namespace cactus::ecs {

export using SignatureID = size_t;

export constexpr SignatureID EMPTY_SIGNATURE_ID = (SignatureID)-1;

export struct SignatureAtlas {
    struct SignatureData {
        ComponentID *ptr;
        size_t size;

        bool operator==(const SignatureData &other) const {
            if (size != other.size) return false;
            return std::equal(ptr, ptr + size, other.ptr);
        }

        struct Hasher {
            auto operator()(const SignatureData &t) const -> size_t {
                return boost::hash_range(t.ptr, t.ptr + t.size);
            }
        };
    };

    struct SignatureCacheTransition {
        SignatureID from;
        ComponentID component;

        auto operator<=>(const SignatureCacheTransition &) const = default;

        struct Hasher {
            auto operator()(const SignatureCacheTransition &t) const -> size_t {
                size_t res = 0;
                boost::hash_combine(res, t.from);
                boost::hash_combine(res, t.component);
                return res;
            }
        };
    };

    bstc::vector<SignatureData> signature_datas;

    bstu::unordered_flat_map<SignatureData, SignatureID, SignatureData::Hasher> data_to_id_map;

    bstu::unordered_flat_map<SignatureCacheTransition, SignatureID,
                             SignatureCacheTransition::Hasher>
        signature_add_component_cache;
    bstu::unordered_flat_map<SignatureCacheTransition, SignatureID,
                             SignatureCacheTransition::Hasher>
        signature_remove_component_cache;

    SignatureAtlas() = default;
    SignatureAtlas(const SignatureAtlas &) = delete;
    SignatureAtlas &operator=(const SignatureAtlas &) = delete;
    SignatureAtlas(SignatureAtlas &&) noexcept = default;
    SignatureAtlas &operator=(SignatureAtlas &&) noexcept = default;
    ~SignatureAtlas() {
        for (auto &c : signature_datas) {
            free(c.ptr);
        }
    }

    [[nodiscard]] auto get_data(SignatureID id) -> SignatureData * {
        assert(id < signature_datas.size());
        return &signature_datas[id];
    }
    [[nodiscard]] auto get_data(SignatureID id) const -> const SignatureData * {
        assert(id < signature_datas.size());
        return &signature_datas[id];
    }

    [[nodiscard]] auto component_index(SignatureID sid, ComponentID cid) const -> size_t {
        assert(sid < signature_datas.size());
        assert(sid != EMPTY_SIGNATURE_ID);
        for (size_t i = 0; i < signature_datas[sid].size; ++i) {
            if (signature_datas[sid].ptr[i] == cid) return i;
        }
        assert(false);
    }

    [[nodiscard]] auto contains_component(SignatureID sid, ComponentID cid) const -> bool {
        if (sid == EMPTY_SIGNATURE_ID) return false;
        assert(sid < signature_datas.size());
        for (size_t i = 0; i < signature_datas[sid].size; ++i) {
            if (signature_datas[sid].ptr[i] == cid) return true;
        }
        return false;
    }

    [[nodiscard]] auto create_or_get_signature_by_add_component(SignatureID existed_id,
                                                                ComponentID cid) -> SignatureID {
        if (auto cache_iter = signature_add_component_cache.find({existed_id, cid});
            cache_iter != signature_add_component_cache.end()) {
            return cache_iter->second;
        }

        if (existed_id == EMPTY_SIGNATURE_ID) {
            signature_datas.emplace_back(
                SignatureData{(ComponentID *)malloc(1 * sizeof(ComponentID)), 1});
            signature_datas.back().ptr[0] = cid;

            signature_add_component_cache.emplace(SignatureCacheTransition{existed_id, cid},
                                                  signature_datas.size() - 1);
            signature_remove_component_cache.emplace(
                SignatureCacheTransition{signature_datas.size() - 1, cid}, existed_id);

            return signature_datas.size() - 1;
        }

        assert(existed_id < signature_datas.size());
        assert(!contains_component(existed_id, cid));

        const auto &old_data = signature_datas[existed_id];

        size_t index = 0;
        while (index < old_data.size) {
            if (old_data.ptr[index] > cid) break;
            ++index;
        }

        const auto new_size = old_data.size + 1;
        auto *temp_new_data = (ComponentID *)alloca(new_size * sizeof(ComponentID));
        memcpy(&temp_new_data[0], &old_data.ptr[0], index * sizeof(ComponentID));
        temp_new_data[index] = cid;
        memcpy(&temp_new_data[index + 1], &old_data.ptr[index],
               (old_data.size - index) * sizeof(ComponentID));

        auto temp_new_signature_id_iter =
            data_to_id_map.find(SignatureData{temp_new_data, new_size});
        size_t new_id;
        if (temp_new_signature_id_iter == data_to_id_map.end()) {
            signature_datas.emplace_back(
                SignatureData{(ComponentID *)malloc(new_size * sizeof(ComponentID)), new_size});
            auto &new_data = signature_datas.back();
            memcpy(&new_data.ptr[0], temp_new_data, new_size * sizeof(size_t));

            new_id = signature_datas.size() - 1;
            data_to_id_map.emplace(new_data, new_id);
        } else {
            new_id = temp_new_signature_id_iter->second;
        }

        signature_add_component_cache.emplace(SignatureCacheTransition{existed_id, cid}, new_id);

        return new_id;
    }

    [[nodiscard]] auto create_or_get_signature_by_remove_component(SignatureID existed_id,
                                                                   ComponentID cid) -> SignatureID {
        auto cache_iter = signature_remove_component_cache.find({existed_id, cid});
        if (cache_iter != signature_remove_component_cache.end()) {
            return cache_iter->second;
        }

        assert(existed_id < signature_datas.size());
        assert(contains_component(existed_id, cid));

        const auto &old_data = signature_datas[existed_id];

        const auto new_size = old_data.size - 1;
        auto *temp_new_data = (size_t *)alloca(new_size * sizeof(size_t));

        for (size_t i = 0; i < old_data.size; ++i) {
            if (old_data.ptr[i] == cid) {
                memcpy(&temp_new_data[0], &old_data.ptr[0], i * sizeof(size_t));
                memcpy(&temp_new_data[i], &old_data.ptr[i + 1], (new_size - i) * sizeof(size_t));
                break;
            }
        }

        auto temp_new_signature_id_iter =
            data_to_id_map.find(SignatureData{temp_new_data, new_size});
        size_t new_id;
        if (temp_new_signature_id_iter == data_to_id_map.end()) {
            signature_datas.emplace_back(
                SignatureData{(ComponentID *)malloc(new_size * sizeof(ComponentID)), new_size});
            auto &new_data = signature_datas.back();
            memcpy(&new_data.ptr[0], temp_new_data, new_size * sizeof(size_t));

            new_id = signature_datas.size() - 1;
            data_to_id_map.emplace(new_data, new_id);
        } else {
            new_id = temp_new_signature_id_iter->second;
        }

        signature_remove_component_cache.emplace(SignatureCacheTransition{existed_id, cid}, new_id);

        return new_id;
    }
};

} // namespace cactus::ecs
