module;

#include <cassert>

export module cactus.core.ecs:signature;

import :component;

import std;

using size_t = std::size_t;

namespace cactus {

export using Signature = std::bitset<MAX_COMPONENT_COUNT>;

export struct SignatureView {
    const Signature *signature_ref;

    explicit SignatureView(const Signature &b) : signature_ref(&b) {}

    struct iterator {
        using iterator_concept = std::forward_iterator_tag;
        using value_type = ComponentKey;
        using difference_type = std::ptrdiff_t;

        const Signature *source;
        ComponentKey cur_component;

        iterator(const Signature &signature, ComponentKey component) : source(&signature), cur_component(component) {
            if (cur_component >= MAX_COMPONENT_COUNT) {
                cur_component = MAX_COMPONENT_COUNT;
            } else if (!source->test(cur_component)) {
                ++(*this);
            }
        }

        auto operator*() const -> value_type { return cur_component; }

        bool operator!=(const iterator &other) const { return cur_component != other.cur_component; }

        auto operator++() -> iterator & {
            do { ++cur_component; } while (cur_component < MAX_COMPONENT_COUNT && !source->test(cur_component));
            return *this;
        }
    };

    iterator begin() const { return iterator(*signature_ref, 0); }
    iterator end() const { return iterator(*signature_ref, MAX_COMPONENT_COUNT); }
};

export using SignatureAtlasKey = size_t;

export constexpr SignatureAtlasKey EMPTY_SIGNATURE_KEY = 0;

export struct SignatureAtlas {
    std::vector<Signature> signatures;

    struct TransitionHash {
        template <typename T, typename U> size_t operator()(const std::pair<T, U> &p) const {
            size_t h1 = std::hash<T>{}(p.first);
            size_t h2 = std::hash<U>{}(p.second);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    using TransitionCacheMapType =
        std::unordered_map<std::pair<SignatureAtlasKey, ComponentKey>, SignatureAtlasKey, TransitionHash>;
    TransitionCacheMapType add_transition_cache_map;
    TransitionCacheMapType remove_transition_cache_map;

    [[nodiscard]] static auto make() -> SignatureAtlas {
        return SignatureAtlas{.signatures = std::vector<Signature>(1),
                              .add_transition_cache_map = TransitionCacheMapType(),
                              .remove_transition_cache_map = TransitionCacheMapType()};
    }

    auto destroy() const {}

    [[nodiscard]] auto has(SignatureAtlasKey key) const -> bool { return key < signatures.size(); }
    [[nodiscard]] auto get(SignatureAtlasKey key) const -> Signature {
        assert(key < signatures.size());
        return signatures[key];
    }
    [[nodiscard]] auto get_ptr(SignatureAtlasKey key) -> Signature * {
        assert(key < signatures.size());
        return &signatures[key];
    }

    [[nodiscard]] auto get_by_add(SignatureAtlasKey key, ComponentKey component) const -> std::optional<SignatureAtlasKey> {
        assert(key < signatures.size());
        assert(component < MAX_COMPONENT_COUNT);

        if (signatures[key].test(component)) return key;

        auto cache_key = std::make_pair(key, component);
        auto cache_it = add_transition_cache_map.find(cache_key);
        if (cache_it != add_transition_cache_map.end()) return cache_it->second;

        Signature new_signature = signatures[key];
        new_signature.set(component);

        SignatureAtlasKey new_key = signatures.size();

        for (size_t i = 0; i < signatures.size(); ++i) {
            if (signatures[i] == new_signature) {
                new_key = i;

                break;
            }
        }

        if (new_key >= signatures.size()) return {};

        return new_key;
    }

    [[nodiscard]] auto get_or_create_by_add(SignatureAtlasKey key, ComponentKey component) -> SignatureAtlasKey {
        assert(key < signatures.size());
        assert(component < MAX_COMPONENT_COUNT);

        if (signatures[key].test(component)) return key;

        auto cache_key = std::make_pair(key, component);
        auto cache_it = add_transition_cache_map.find(cache_key);
        if (cache_it != add_transition_cache_map.end()) return cache_it->second;

        Signature new_signature = signatures[key];
        new_signature.set(component);

        SignatureAtlasKey new_key = signatures.size();

        for (size_t i = 0; i < signatures.size(); ++i) {
            if (signatures[i] == new_signature) {
                add_transition_cache_map[cache_key] = i;
                new_key = i;

                break;
            }
        }

        if (new_key >= signatures.size()) signatures.push_back(new_signature);

        add_transition_cache_map[cache_key] = new_key;
        remove_transition_cache_map[std::make_pair(new_key, component)] = key;

        return new_key;
    }

    [[nodiscard]] auto get_by_remove(SignatureAtlasKey key, ComponentKey component) const -> std::optional<SignatureAtlasKey> {
        assert(key < signatures.size());
        assert(component < MAX_COMPONENT_COUNT);

        if (!signatures[key].test(component)) return key;

        auto cache_key = std::make_pair(key, component);
        auto cache_it = remove_transition_cache_map.find(cache_key);
        if (cache_it != remove_transition_cache_map.end()) return cache_it->second;

        Signature new_signature = signatures[key];
        new_signature.reset(component);

        SignatureAtlasKey new_key = signatures.size();

        if (new_signature.none()) {
            new_key = EMPTY_SIGNATURE_KEY;
        } else {
            for (size_t i = 0; i < signatures.size(); ++i) {
                if (signatures[i] == new_signature) {
                    new_key = i;

                    break;
                }
            }
        }

        if (new_key >= signatures.size()) return {};

        return new_key;
    }

    [[nodiscard]] auto get_or_create_by_remove(SignatureAtlasKey key, ComponentKey component) -> SignatureAtlasKey {
        assert(key < signatures.size());
        assert(component < MAX_COMPONENT_COUNT);

        if (!signatures[key].test(component)) return key;

        auto cache_key = std::make_pair(key, component);
        auto cache_it = remove_transition_cache_map.find(cache_key);
        if (cache_it != remove_transition_cache_map.end()) return cache_it->second;

        Signature new_signature = signatures[key];
        new_signature.reset(component);

        SignatureAtlasKey new_key = signatures.size();

        if (new_signature.none()) {
            new_key = EMPTY_SIGNATURE_KEY;
        } else {
            for (size_t i = 0; i < signatures.size(); ++i) {
                if (signatures[i] == new_signature) {
                    remove_transition_cache_map[cache_key] = i;
                    new_key = i;

                    break;
                }
            }
        }

        if (new_key >= signatures.size()) signatures.push_back(new_signature);

        remove_transition_cache_map[cache_key] = new_key;
        add_transition_cache_map[std::make_pair(new_key, component)] = key;

        return new_key;
    }

    [[nodiscard]] auto signature_test(SignatureAtlasKey key, ComponentKey component) const -> bool {
        assert(key < signatures.size());
        return signatures[key].test(component);
    }

    [[nodiscard]] auto signature_none(SignatureAtlasKey key) const -> bool {
        assert(key < signatures.size());
        return signatures[key].none();
    }

    auto signature_set(SignatureAtlasKey key, ComponentKey component) {
        assert(key < signatures.size());
        signatures[key].set(component);
    }

    auto signature_reset(SignatureAtlasKey key, ComponentKey component) {
        assert(key < signatures.size());
        signatures[key].reset(component);
    }

    auto signature_count(SignatureAtlasKey key) const -> size_t {
        assert(key < signatures.size());
        return signatures[key].count();
    }
};

} // namespace cactus
