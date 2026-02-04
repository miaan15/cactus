module;

#include <boost/container/vector.hpp>
#include <cstdlib>
#include <utility>

export module Physics;

import glm;
import SlotMap;

using namespace glm;
namespace bstc = boost::container;

auto operator==(const std::pair<unsigned, unsigned> &a, const std::pair<unsigned, unsigned> &b)
    -> bool {
    return a.first == b.first && a.second == b.second;
}

namespace cactus {

export using Collider = mat2x2;
export using AABB = mat2x2;

export struct ColliderEntry {
    Collider collider;

    void *node_ptr;
};
export using ColliderEntrySet = slot_map<ColliderEntry>;
export using ColliderEntryKey = ColliderEntrySet::key_type;

namespace _private {
struct Node {
    Node *parent;
    Node *childs[2];

    ColliderEntryKey key;

    AABB aabb;

    uint8_t flag : 7;
    uint8_t is_self_check : 1;
};
struct FitNodeVal {
    Node *node;
    Node **link;
    float value;
};
} // namespace _private
using namespace _private;

export struct PhysicsWorld {
    float margin;
    ColliderEntrySet entries;

    Node *_root;

// private:
    auto _tree_insert(const Collider &collider, uint8_t flag = 0) -> ColliderEntryKey {
        if (_root == nullptr) {
            auto *node = (Node *)malloc(sizeof(Node));
            auto key = entries.emplace(collider, node);
            *node = {.key = key, .aabb = aabb_expand_margin(collider, margin), .flag = flag};

            _root = node;

            return key;
        }

        AABB fat_aabb = aabb_expand_margin(collider, margin);

        FitNodeVal best{
            .node = _root, .link = &_root, .value = aabb_volume(aabb_merge(fat_aabb, _root->aabb))};
        _tree_find_best_fitnode_helper(&best, fat_aabb, 0.0, _root, &_root);

        auto *node = (Node *)malloc(sizeof(Node));
        auto key = entries.emplace(collider, node);
        *node = {.key = key, .aabb = fat_aabb, .flag = flag};

        auto *parent = (Node *)malloc(sizeof(Node));
        *parent = {.parent = best.node->parent,
                   .childs = {best.node, node},
                   .aabb = aabb_merge(best.node->aabb, fat_aabb),
                   .flag = static_cast<uint8_t>(best.node->flag & node->flag)};

        best.node->parent = parent;
        node->parent = parent;
        *best.link = parent;

        auto *ucur_node = parent->parent;
        while (ucur_node != nullptr) {
            ucur_node->flag = ucur_node->childs[0]->flag & ucur_node->childs[1]->flag;
            ucur_node->aabb = aabb_merge(ucur_node->childs[0]->aabb, ucur_node->childs[1]->aabb);
            ucur_node = ucur_node->parent;
        }

        return key;
    }

    auto _tree_remove(ColliderEntryKey key) -> bool {
        if (_root == nullptr) return false;
        if (_tree_is_node_leaf(*_root)) {
            if (key == _root->key) {
                _root = nullptr;
                free(_root);
                return true;
            }
            return false;
        }
        return _tree_remove_helper(key, _root, &_root);
    }

    auto _tree_update() -> void {
        if (_root == nullptr) return;
        if (_tree_is_node_leaf(*_root)) {
            auto &collider = get_entry(_root->key)->collider;
            if (!aabb_contains(_root->aabb, collider)) {
                _root->aabb = aabb_expand_margin(collider, margin);
            }
            return;
        }

        bstc::vector<Node *> invalid_list{};
        _tree_get_invalid_nodes_helper(&invalid_list, _root);

        for (int i = 0; i < invalid_list.size(); i++) {
            auto *node = (Node *)invalid_list[i];
            auto *parent = node->parent;
            auto *sibling = node == parent->childs[0] ? parent->childs[1] : parent->childs[0];
            auto **parent_link =
                parent->parent ? (parent == parent->parent->childs[0] ? &parent->parent->childs[0]
                                                                      : &parent->parent->childs[1])
                               : &_root;

            sibling->parent = parent->parent ? parent->parent : nullptr;
            *parent_link = sibling;

            free(parent);

            _tree_handle_reinsert_node(node);
        }
    }

    auto _tree_get_collided_pairs() -> bstc::vector<std::pair<ColliderEntryKey, ColliderEntryKey>> {
        bstc::vector<std::pair<ColliderEntryKey, ColliderEntryKey>> res{};
        if (_root == nullptr) return res;
        if (_tree_is_node_leaf(*_root)) return res;

        _tree_uncheck_selfcheck_flag_helper(_root);
        _tree_get_collided_pairs_helper(&res, _root->childs[0], _root->childs[1]);

        return res;
    }

    ~PhysicsWorld() {
        _tree_free_helper(_root);
    }

private:
    auto get_entry(ColliderEntryKey key) -> ColliderEntry * {
        return &entries.at(key);
    }

    auto aabb_merge(const AABB &a, const AABB &b) -> AABB {
        return AABB(min(a[0], b[0]), max(a[1], b[1]));
    }
    auto aabb_intersects(const AABB &a, const AABB &b) -> bool {
        return all(lessThanEqual(a[0], b[1])) && all(lessThanEqual(b[0], a[1]));
    }
    auto aabb_contains(const AABB &a, const AABB &b) -> bool {
        return all(lessThanEqual(a[0], b[0])) && all(lessThanEqual(b[1], a[1]));
    }
    auto aabb_volume(const AABB &a) -> float {
        vec2 size = a[1] - a[0];
        return size.x * size.y;
    }
    auto aabb_expand_margin(const AABB &a, float margin) -> AABB {
        return AABB(a[0] - vec2(margin, margin), a[1] + vec2(margin, margin));
    }

    auto _tree_is_node_leaf(const Node &node) -> bool {
        return node.childs[0] == nullptr;
    }

    auto _tree_find_best_fitnode_helper(FitNodeVal *best, AABB aabb, float acml_d, Node *cur,
                                       Node **cur_link) -> void {
        AABB merge_aabb = aabb_merge(aabb, cur->aabb);

        float cur_value = aabb_volume(merge_aabb) + acml_d;
        if (cur_value < best->value) {
            best->node = cur;
            best->link = cur_link;
            best->value = cur_value;
        }

        if (_tree_is_node_leaf(*cur)) return;

        float cur_delta = aabb_volume(merge_aabb) - aabb_volume(cur->aabb);
        if (aabb_volume(aabb) + cur_delta + acml_d < best->value) {
            _tree_find_best_fitnode_helper(best, aabb, acml_d + cur_delta, cur->childs[0],
                                          &cur->childs[0]);
            _tree_find_best_fitnode_helper(best, aabb, acml_d + cur_delta, cur->childs[1],
                                          &cur->childs[1]);
        }
    }

    bool _tree_remove_helper(ColliderEntryKey entry_key, Node *cur, Node **cur_link) {
        bool res = false;
        if (!_tree_is_node_leaf(*cur->childs[0])) {
            if (aabb_contains(cur->childs[0]->aabb, get_entry(entry_key)->collider)) {
                res |= _tree_remove_helper(entry_key, cur->childs[0], &cur->childs[0]);
            }
        } else {
            if (get_entry(cur->childs[0]->key)->collider == get_entry(entry_key)->collider) {
                *cur_link = cur->childs[1];
                cur->childs[1]->parent = cur->parent;
                free(cur->childs[0]);
                free(cur);
                return true;
            }
        }

        if (!_tree_is_node_leaf(*cur->childs[1])) {
            if (aabb_contains(cur->childs[1]->aabb, get_entry(entry_key)->collider)) {
                res |= _tree_remove_helper(entry_key, cur->childs[1], &cur->childs[1]);
            }
        } else {
            if (get_entry(cur->childs[1]->key)->collider == get_entry(entry_key)->collider) {
                *cur_link = cur->childs[0];
                cur->childs[0]->parent = cur->parent;
                free(cur->childs[1]);
                free(cur);
                return true;
            }
        }

        if (res) {
            cur->aabb = aabb_merge(cur->childs[0]->aabb, cur->childs[1]->aabb);
            cur->flag = cur->childs[0]->flag & cur->childs[1]->flag;
        }

        return res;
    }

    void _tree_get_invalid_nodes_helper(bstc::vector<Node *> *invalid_list, Node *cur) {
        if (_tree_is_node_leaf(*cur)) {
            if (!aabb_contains(cur->aabb, get_entry(cur->key)->collider)) {
                invalid_list->emplace_back(cur);
            }
        } else {
            _tree_get_invalid_nodes_helper(invalid_list, cur->childs[0]);
            _tree_get_invalid_nodes_helper(invalid_list, cur->childs[1]);
        }
    }

    void _tree_handle_reinsert_node(Node *node) {
        if (_root == nullptr) {
            auto *node = (Node *)malloc(sizeof(Node));
            *node = {};

            _root = node;

            return;
        }

        auto &collider = get_entry(node->key)->collider;
        AABB fat_aabb = aabb_expand_margin(collider, margin);

        FitNodeVal best{
            .node = _root, .link = &_root, .value = aabb_volume(aabb_merge(fat_aabb, _root->aabb))};
        _tree_find_best_fitnode_helper(&best, fat_aabb, 0.0, _root, &_root);

        node->aabb = fat_aabb;

        auto *parent = (Node *)malloc(sizeof(Node));
        *parent = {.parent = best.node->parent,
                   .childs = {best.node, node},
                   .aabb = aabb_merge(best.node->aabb, node->aabb),
                   .flag = static_cast<uint8_t>(best.node->flag & node->flag)};

        best.node->parent = parent;
        node->parent = parent;
        *best.link = parent;

        auto *ucur_node = parent->parent;
        while (ucur_node != nullptr) {
            ucur_node->flag = ucur_node->childs[0]->flag & ucur_node->childs[1]->flag;
            ucur_node->aabb = aabb_merge(ucur_node->childs[0]->aabb, ucur_node->childs[1]->aabb);
            ucur_node = ucur_node->parent;
        }
    }

    void _tree_uncheck_selfcheck_flag_helper(Node *cur) {
        cur->is_self_check = false;

        if (_tree_is_node_leaf(*cur)) return;
        _tree_uncheck_selfcheck_flag_helper(cur->childs[0]);
        _tree_uncheck_selfcheck_flag_helper(cur->childs[1]);
    }

    void
    _tree_handle_self_collide_pair(bstc::vector<std::pair<ColliderEntryKey, ColliderEntryKey>> *list,
                                  Node *node) {
        if (!node->is_self_check) {
            _tree_get_collided_pairs_helper(list, node->childs[0], node->childs[1]);
            node->is_self_check = true;
        }
    }
    void _tree_get_collided_pairs_helper(
        bstc::vector<std::pair<ColliderEntryKey, ColliderEntryKey>> *list, Node *node0,
        Node *node1) {
        if ((node0->flag & node1->flag) != 0) return;

        if (_tree_is_node_leaf(*node0) && _tree_is_node_leaf(*node1)) {
            if (aabb_intersects(get_entry(node0->key)->collider, get_entry(node1->key)->collider)) {
                list->emplace_back(node0->key, node1->key);
            }
            return;
        }

        if (!aabb_intersects(node0->aabb, node1->aabb)) {
            if (!_tree_is_node_leaf(*node0) && node0->flag == 0)
                _tree_handle_self_collide_pair(list, node0);
            if (!_tree_is_node_leaf(*node1) && node1->flag == 0)
                _tree_handle_self_collide_pair(list, node1);

            return;
        }

        if (_tree_is_node_leaf(*node0)) {
            if (node1->flag == 0) _tree_handle_self_collide_pair(list, node1);

            _tree_get_collided_pairs_helper(list, node0, node1->childs[0]);
            _tree_get_collided_pairs_helper(list, node0, node1->childs[1]);

            return;
        }
        if (_tree_is_node_leaf(*node1)) {
            if (node0->flag == 0) _tree_handle_self_collide_pair(list, node0);

            _tree_get_collided_pairs_helper(list, node0->childs[0], node1);
            _tree_get_collided_pairs_helper(list, node0->childs[1], node1);

            return;
        }

        if (node0->flag == 0) _tree_handle_self_collide_pair(list, node0);
        if (node1->flag == 0) _tree_handle_self_collide_pair(list, node1);

        _tree_get_collided_pairs_helper(list, node0->childs[0], node1->childs[0]);
        _tree_get_collided_pairs_helper(list, node0->childs[0], node1->childs[1]);
        _tree_get_collided_pairs_helper(list, node0->childs[1], node1->childs[0]);
        _tree_get_collided_pairs_helper(list, node0->childs[1], node1->childs[1]);
    }

    void _tree_free_helper(Node *cur) {
        if (cur == nullptr) return;

        if (cur->childs[0] != nullptr) _tree_free_helper(cur->childs[0]);
        if (cur->childs[1] != nullptr) _tree_free_helper(cur->childs[1]);

        free(cur);
    }
};

} // namespace cactus
