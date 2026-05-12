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

    [[nodiscard]] static auto make() -> SignatureAtlas { return SignatureAtlas{.signatures = std::vector<Signature>(1)}; }

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

    [[nodiscard]] auto get_key_by_add(SignatureAtlasKey key, std::initializer_list<ComponentKey> components) const
        -> std::optional<SignatureAtlasKey> {
        Signature new_signature = signatures[key];
        for (ComponentKey component : components) { new_signature.set(component); }

        for (size_t i = 0; i < signatures.size(); i++) {
            if (signatures[i] == new_signature) return i;
        }
        return {};
    }
    [[nodiscard]] auto get_or_create_key_by_add(SignatureAtlasKey key, std::initializer_list<ComponentKey> components)
        -> SignatureAtlasKey {
        Signature new_signature = signatures[key];
        for (ComponentKey component : components) { new_signature.set(component); }

        for (size_t i = 0; i < signatures.size(); i++) {
            if (signatures[i] == new_signature) return i;
        }

        signatures.push_back(std::move(new_signature));
        return signatures.size() - 1;
    }

    [[nodiscard]] auto get_key_by_remove(SignatureAtlasKey key, std::initializer_list<ComponentKey> components) const
        -> std::optional<SignatureAtlasKey> {
        Signature new_signature = signatures[key];
        for (ComponentKey component : components) { new_signature.reset(component); }

        for (size_t i = 0; i < signatures.size(); i++) {
            if (signatures[i] == new_signature) return i;
        }
        return {};
    }
    [[nodiscard]] auto get_or_create_key_by_remove(SignatureAtlasKey key, std::initializer_list<ComponentKey> components)
        -> SignatureAtlasKey {
        Signature new_signature = signatures[key];
        for (ComponentKey component : components) { new_signature.reset(component); }

        for (size_t i = 0; i < signatures.size(); i++) {
            if (signatures[i] == new_signature) return i;
        }

        signatures.push_back(std::move(new_signature));
        return signatures.size() - 1;
    }

    [[nodiscard]] auto get_key_from_signature(Signature &&signature) const -> std::optional<SignatureAtlasKey> {
        for (size_t i = 0; i < signatures.size(); i++) {
            if (signatures[i] == signature) return i;
        }
        return {};
    }
    [[nodiscard]] auto get_or_create_key_from_signature(Signature &&signature) -> SignatureAtlasKey {
        for (size_t i = 0; i < signatures.size(); i++) {
            if (signatures[i] == signature) return i;
        }
        signatures.push_back(std::move(signature));
        return signatures.size() - 1;
    }

    [[nodiscard]] auto signature_test(SignatureAtlasKey key, ComponentKey component) const -> bool {
        assert(key < signatures.size());
        return signatures[key].test(component);
    }

    [[nodiscard]] auto signature_none(SignatureAtlasKey key) const -> bool {
        assert(key < signatures.size());
        return signatures[key].none();
    }

    [[nodiscard]] auto signature_count(SignatureAtlasKey key) const -> size_t {
        assert(key < signatures.size());
        return signatures[key].count();
    }

    [[nodiscard]] auto signature_contains(SignatureAtlasKey key, SignatureAtlasKey other_key) const -> bool {
        assert(key < signatures.size());
        assert(other_key < signatures.size());
        return (signatures[key] & signatures[other_key]) == signatures[other_key];
    }
};

} // namespace cactus
