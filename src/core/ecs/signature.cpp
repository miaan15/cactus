module;

#include <cassert>

export module cactus.core.ecs:signature;

import std;

using size_t = std::size_t;

namespace cactus {

export struct Signature {
    std::type_index *data_raw;
    size_t len;

    bool operator==(const Signature &other) const {
        if (len != other.len) return false;
        return std::equal(data_raw, data_raw + len, other.data_raw);
    }

    struct Hasher {
        auto operator()(const Signature &s) const -> size_t {
            if (!s.data_raw || s.len == 0) return 0;

            size_t res = s.data_raw[0].hash_code();
            for (size_t i = 1; i < s.len; i++) {
                size_t h = s.data_raw[i].hash_code();
                res ^= h + 0x9e3779b9 + (res << 6) + (res >> 2);
            }

            res ^= std::hash<size_t>{}(s.len) + 0x9e3779b9 + (res << 6) + (res >> 2);

            return res;
        }
    };

    [[nodiscard]] static auto make() -> Signature { return Signature{.data_raw = nullptr, .len = 0}; }

    auto destroy() const { std::free(this->data_raw); }

    auto clone() const -> Signature {
        std::type_index *new_data_raw = (std::type_index *)std::malloc(this->len * sizeof(std::type_index));
        std::memcpy(new_data_raw, this->data_raw, this->len * sizeof(std::type_index));
        return Signature{.data_raw = new_data_raw, .len = this->len};
    }

    auto push(std::type_index component) {
        assert(!this->has(component));

        size_t new_len = this->len + 1;
        std::type_index *new_data_raw = (std::type_index *)std::malloc(new_len * sizeof(std::type_index));
        bool flag = 0;
        for (size_t i = 0; i < this->len; i++) {
            if (!flag && component < this->data_raw[i]) {
                new_data_raw[i] = component;
                flag = 1;
            }
            new_data_raw[i + flag] = this->data_raw[i];
        }
        if (!flag) { new_data_raw[new_len - 1] = component; }

        std::free(this->data_raw);

        this->data_raw = new_data_raw;
        this->len = new_len;
    }
    auto push(std::initializer_list<std::type_index> component_list) {
        assert(!this->is_dup_components(component_list));

        std::vector<std::type_index> components{component_list};
        std::sort(components.begin(), components.end());

        size_t new_len = this->len + components.size();
        std::type_index *new_data_raw = (std::type_index *)std::malloc(new_len * sizeof(std::type_index));
        size_t i = 0, j = 0;
        while (i < this->len && j < components.size()) {
            if (components[j] < this->data_raw[i]) {
                new_data_raw[i + j] = components[j];
                ++j;
            } else {
                new_data_raw[i + j] = this->data_raw[i];
                ++i;
            }
        }
        while (i < this->len) {
            new_data_raw[i + j] = this->data_raw[i];
            ++i;
        }
        while (j < components.size()) {
            new_data_raw[i + j] = components[j];
            ++j;
        }

        std::free(this->data_raw);

        this->data_raw = new_data_raw;
        this->len = new_len;
    }

    auto remove(std::type_index component) {
        assert(this->has(component));

        if (this->len < 1) return;

        size_t new_len = this->len - 1;
        std::type_index *new_data_raw = (std::type_index *)std::malloc(new_len * sizeof(std::type_index));

        bool flag = 0;
        for (size_t i = 0; i < this->len; i++) {
            if (!flag && component == this->data_raw[i]) {
                flag = 1;
                continue;
            }
            new_data_raw[i - flag] = this->data_raw[i];
        }

        std::free(this->data_raw);

        this->data_raw = new_data_raw;
        this->len = new_len;
    }
    auto remove(std::initializer_list<std::type_index> component_list) {
        assert(this->is_all_dup_components(component_list));

        std::vector<std::type_index> components{component_list};
        if (components.size() > this->len) return;

        std::sort(components.begin(), components.end());

        size_t new_len = this->len - components.size();
        std::type_index *new_data_raw = (std::type_index *)std::malloc(new_len * sizeof(std::type_index));
        size_t i = 0, j = 0, d = 0;
        while (i < this->len && j < components.size()) {
            if (this->data_raw[i] == components[j]) {
                ++j;
                ++i;
            } else {
                new_data_raw[d] = this->data_raw[i];
                ++d;
                ++i;
            }
        }
        while (i < this->len) {
            new_data_raw[d] = this->data_raw[i];
            ++d;
            ++i;
        }

        std::free(this->data_raw);

        this->data_raw = new_data_raw;
        this->len = new_len;
    }

    [[nodiscard]] auto has(std::type_index component) const -> bool {
        for (size_t i = 0; i < this->len; i++) {
            if (component == this->data_raw[i]) return true;
        }
        return false;
    }

    [[nodiscard]] auto get(size_t index) const -> std::optional<std::type_index> {
        if (index >= this->len) return {};
        return this->data_raw[index];
    }

private:
    [[nodiscard]] auto is_dup_components(std::initializer_list<std::type_index> components) -> bool {
        for (const auto &c : components) {
            if (this->has(c)) return true;
        }
        return false;
    }
    [[nodiscard]] auto is_all_dup_components(std::initializer_list<std::type_index> components) -> bool {
        for (const auto &c : components) {
            if (!this->has(c)) return false;
        }
        return true;
    }
};

export using SignatureAtlasKey = size_t;
export constexpr SignatureAtlasKey EMPTY_SIGNATURE_KEY = (SignatureAtlasKey)-1;

export struct SignatureTransition {
    SignatureAtlasKey from;
    std::type_index component;

    bool operator==(const SignatureTransition &other) const { return from == other.from && component == other.component; }

    struct Hasher {
        auto operator()(const SignatureTransition &st) const -> size_t {
            size_t res = std::hash<SignatureAtlasKey>{}(st.from);
            res ^= st.component.hash_code() + 0x9e3779b9 + (res << 6) + (res >> 2);

            return res;
        }
    };
};

export struct SignatureAtlas {
    std::vector<Signature> signatures;
    std::unordered_map<Signature, SignatureAtlasKey, Signature::Hasher> signature_to_key_map;

    using TransitionMap = std::unordered_map<SignatureTransition, SignatureAtlasKey, SignatureTransition::Hasher>;
    TransitionMap transitions_by_add;
    TransitionMap transitions_by_remove;

    [[nodiscard]] static auto make() -> SignatureAtlas { return SignatureAtlas{}; }

    auto destroy() const {
        for (const auto &s : signatures) s.destroy();
    }

    [[nodiscard]] auto has(SignatureAtlasKey key) const -> bool { return key < signatures.size(); }
    [[nodiscard]] auto get(SignatureAtlasKey key) const -> std::optional<Signature> {
        if (key >= signatures.size()) return {};
        return signatures[key];
    }

    auto new_signature(Signature &&signature) -> size_t {
        signatures.push_back(signature);
        return signatures.size() - 1;
    }

    [[nodiscard]] auto get_key_by_add_component(SignatureAtlasKey key, std::type_index component)
        -> std::optional<SignatureAtlasKey> {
        if (key != EMPTY_SIGNATURE_KEY && !this->has(key)) return {};

        {
            auto to_key_it = this->transitions_by_add.find(SignatureTransition{key, component});
            if (to_key_it != this->transitions_by_add.end()) return to_key_it->second;
        }

        if (key == EMPTY_SIGNATURE_KEY) {
            Signature new_signature = Signature::make();
            new_signature.push(component);
            SignatureAtlasKey new_key = this->new_signature(std::move(new_signature));

            transitions_by_add[SignatureTransition{key, component}] = new_key;

            return new_key;
        }

        Signature cur_signature = this->get(key).value();

        if (cur_signature.has(component)) return {};

        Signature new_signature = cur_signature.clone();
        new_signature.push(component);

        auto existed_new_key_it = this->signature_to_key_map.find(new_signature);
        if (existed_new_key_it != this->signature_to_key_map.end()) {
            new_signature.destroy();

            SignatureAtlasKey new_key = existed_new_key_it->second;

            transitions_by_add[SignatureTransition{key, component}] = new_key;

            return new_key;
        }

        SignatureAtlasKey new_key = this->new_signature(std::move(new_signature));

        transitions_by_add[SignatureTransition{key, component}] = new_key;

        return new_key;
    }

    [[nodiscard]] auto get_key_by_remove_component(SignatureAtlasKey key, std::type_index component)
        -> std::optional<SignatureAtlasKey> {
        if (key == EMPTY_SIGNATURE_KEY || !this->has(key)) return {};

        {
            auto to_key_it = this->transitions_by_remove.find(SignatureTransition{key, component});
            if (to_key_it != this->transitions_by_remove.end()) return to_key_it->second;
        }

        Signature cur_signature = this->get(key).value();

        if (!cur_signature.has(component)) return {};

        if (cur_signature.len == 1) {
            SignatureAtlasKey new_key = EMPTY_SIGNATURE_KEY;

            transitions_by_remove[SignatureTransition{key, component}] = new_key;

            return new_key;
        }

        Signature new_signature = cur_signature.clone();
        new_signature.remove(component);

        auto existed_new_key_it = this->signature_to_key_map.find(new_signature);
        if (existed_new_key_it != this->signature_to_key_map.end()) {
            new_signature.destroy();

            SignatureAtlasKey new_key = existed_new_key_it->second;

            transitions_by_remove[SignatureTransition{key, component}] = new_key;

            return new_key;
        }

        SignatureAtlasKey new_key = this->new_signature(std::move(new_signature));

        transitions_by_remove[SignatureTransition{key, component}] = new_key;

        return new_key;
    }
};

} // namespace cactus
