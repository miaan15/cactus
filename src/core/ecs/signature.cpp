module;

#include <cassert>

export module cactus.core.ecs:signature;

import :component;

import std;

using size_t = std::size_t;

namespace cactus {

export using Signature = std::bitset<MAX_COMPONENT_COUNT>;

export struct SignatureView {
    const Signature *data_ref;

    explicit SignatureView(const Signature &b) : data_ref(&b) {}

    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = ComponentKey;
        using difference_type = std::ptrdiff_t;
        using pointer = const ComponentKey *;
        using reference = const ComponentKey &;

        const Signature *source;
        ComponentKey cur_component;

        iterator(const Signature &s, ComponentKey bit) : source(&s), cur_component(bit) {
            if (this->cur_component >= MAX_COMPONENT_COUNT) {
                this->cur_component = MAX_COMPONENT_COUNT;
            } else if (!this->source->test(this->cur_component)) {
                operator++();
            }
        }

        auto operator*() const -> ComponentKey { return this->cur_component; }

        bool operator!=(const iterator &other) const { return this->cur_component != other.cur_component; }

        auto operator++() -> iterator & {
            // TODO can be optimize by using builtin to get first non-zero in uint64
            do {
                ++this->cur_component;
            } while (this->cur_component < MAX_COMPONENT_COUNT && !this->source->test(this->cur_component));
            return *this;
        }
    };

    iterator begin() const { return iterator(*data_ref, 0); }
    iterator end() const { return iterator(*data_ref, MAX_COMPONENT_COUNT); }
};

} // namespace cactus
