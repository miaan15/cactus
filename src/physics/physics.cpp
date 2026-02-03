module;

#include <boost/container/vector.hpp>
#include <cstdlib>
#include <utility>

export module Physics;

import glm;
import SlotMap;

using namespace glm;
namespace bstc = boost::container;

auto operator==(const std::pair<unsigned, unsigned> &a, const std::pair<unsigned, unsigned> &b) -> bool {
    return a.first == b.first && a.second == b.second;
}

namespace cactus {

export using Collider = mat2x2;
export using AABB = mat2x2;

export struct ColliderEntry {
    Collider collider;
};
export using ColliderEntrySet = slot_map<ColliderEntry>;
export using ColliderEntryKey = ColliderEntrySet::key_type;

namespace _private {
enum NodeType { NORMAL = 0, INERT = 1 };
struct Node {
    Node *parent;
    Node *childs[2];

    AABB aabb;
    ColliderEntryKey key;

    uint8_t type : 1;
    uint8_t is_self_check : 1;
    uint8_t __padding : 6;
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

    ColliderEntrySet _collider_entries;
    Node *_root;

    auto tree_insert(const Collider &collider, NodeType type = NORMAL) -> ColliderEntryKey {
        auto entry_key = _collider_entries.emplace(collider);

        if (_root == nullptr) {
            auto *node = (Node *)malloc(sizeof(Node));
            node->parent = node->childs[0] = node->childs[1] = nullptr;
            node->is_self_check = false;
            node->aabb = aabb_expand_margin(collider, margin);
            node->key = entry_key;
            node->type = type;

            _root = node;

            return entry_key;
        }

        AABB fat_aabb = aabb_expand_margin(collider, margin);

        FitNodeVal best{.node = _root, .link = &_root, .value = aabb_volume(aabb_merge(fat_aabb, _root->aabb))};
        tree_find_best_fitnode_helper(&best, fat_aabb, 0.0, _root, &_root);

        auto *new_node = (Node *)malloc(sizeof(Node));
        new_node->childs[0] = new_node->childs[1] = nullptr;
        new_node->is_self_check = false;
        new_node->aabb = fat_aabb;
        new_node->key = entry_key;
        new_node->type = type;

        auto *new_parent = (Node *)malloc(sizeof(Node));
        new_parent->is_self_check = false;
        new_parent->aabb = aabb_merge(best.node->aabb, new_node->aabb);
        new_parent->parent = best.node->parent;
        new_parent->childs[0] = best.node;
        new_parent->childs[1] = new_node;
        new_parent->type = new_parent->childs[0]->type & new_parent->childs[1]->type;

        best.node->parent = new_parent;
        new_node->parent = new_parent;
        *best.link = new_parent;

        auto *ucur_node = new_parent->parent;
        while (ucur_node != nullptr) {
            ucur_node->type = ucur_node->childs[0]->type & ucur_node->childs[1]->type;
            ucur_node->aabb = aabb_merge(ucur_node->childs[0]->aabb, ucur_node->childs[1]->aabb);
            ucur_node = ucur_node->parent;
        }

        return entry_key;
    }

    auto tree_remove(ColliderEntryKey entry_key) -> bool {
        if (_root == nullptr) return false;
        if (tree_is_node_leaf(*_root)) {
            if (entry_key == _root->key) {
                _root = nullptr;
                free(_root);
                return true;
            }
            return false;
        }
        return tree_remove_helper(entry_key, _root, &_root);
    }

    auto tree_update() -> void {
        if (_root == nullptr) return;

        if (tree_is_node_leaf(*_root)) {
            if (!aabb_contains(_root->aabb, get_entry(_root->key)->collider)) {
                _root->aabb = aabb_expand_margin(get_entry(_root->key)->collider, margin);
            }
            return;
        }

        bstc::vector<Node *> invalid_list{};
        tree_get_invalid_nodes_helper(&invalid_list, _root);

        for (int i = 0; i < invalid_list.size(); i++) {
            auto *node = (Node *)invalid_list[i];
            auto *parent = node->parent;
            auto *sibling = node == parent->childs[0] ? parent->childs[1] : parent->childs[0];
            auto **parent_link = parent->parent
                                     ? (parent == parent->parent->childs[0] ? &parent->parent->childs[0] : &parent->parent->childs[1])
                                     : &_root;

            sibling->parent = parent->parent ? parent->parent : nullptr;
            *parent_link = sibling;

            free(parent);

            tree_handle_reinsert_node(node);
        }
    }

    auto tree_get_collided_pairs() -> bstc::vector<std::pair<ColliderEntryKey, ColliderEntryKey>> {
        bstc::vector<std::pair<ColliderEntryKey, ColliderEntryKey>> res{};
        if (_root == nullptr) return res;
        if (tree_is_node_leaf(*_root)) return res;

        tree_uncheck_selfcheck_flag_helper(_root);
        tree_get_collided_pairs_helper(&res, _root->childs[0], _root->childs[1]);

        return res;
    }

    ~PhysicsWorld() {
        tree_free_helper(_root);
    }

private:
    auto get_entry(ColliderEntryKey key) -> ColliderEntry * {
        return &_collider_entries.at(key);
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

    auto tree_is_node_leaf(const Node &node) -> bool {
        return node.childs[0] == nullptr;
    }

    auto tree_find_best_fitnode_helper(FitNodeVal *best, AABB aabb, float acml_d, Node *cur, Node **cur_link) -> void {
        AABB merge_aabb = aabb_merge(aabb, cur->aabb);

        float cur_value = aabb_volume(merge_aabb) + acml_d;
        if (cur_value < best->value) {
            best->node = cur;
            best->link = cur_link;
            best->value = cur_value;
        }

        if (tree_is_node_leaf(*cur)) return;

        float cur_delta = aabb_volume(merge_aabb) - aabb_volume(cur->aabb);
        if (aabb_volume(aabb) + cur_delta + acml_d < best->value) {
            tree_find_best_fitnode_helper(best, aabb, acml_d + cur_delta, cur->childs[0], &cur->childs[0]);
            tree_find_best_fitnode_helper(best, aabb, acml_d + cur_delta, cur->childs[1], &cur->childs[1]);
        }
    }

    bool tree_remove_helper(ColliderEntryKey entry_key, Node *cur, Node **cur_link) {
        bool res = false;
        if (!tree_is_node_leaf(*cur->childs[0])) {
            if (aabb_contains(cur->childs[0]->aabb, get_entry(entry_key)->collider)) {
                res |= tree_remove_helper(entry_key, cur->childs[0], &cur->childs[0]);
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

        if (!tree_is_node_leaf(*cur->childs[1])) {
            if (aabb_contains(cur->childs[1]->aabb, get_entry(entry_key)->collider)) {
                res |= tree_remove_helper(entry_key, cur->childs[1], &cur->childs[1]);
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
            cur->type = cur->childs[0]->type & cur->childs[1]->type;
        }

        return res;
    }

    void tree_get_invalid_nodes_helper(bstc::vector<Node *> *invalid_list, Node *cur) {
        if (tree_is_node_leaf(*cur)) {
            if (!aabb_contains(cur->aabb, get_entry(cur->key)->collider)) {
                invalid_list->emplace_back(cur);
            }
        } else {
            tree_get_invalid_nodes_helper(invalid_list, cur->childs[0]);
            tree_get_invalid_nodes_helper(invalid_list, cur->childs[1]);
        }
    }

    void tree_handle_reinsert_node(Node *node) {
        auto &aabb = get_entry(node->key)->collider;
        if (_root == nullptr) {
            auto *node = (Node *)malloc(sizeof(Node));
            node->parent = nullptr;

            _root = node;

            return;
        }

        AABB fat_aabb = aabb_expand_margin(aabb, margin);

        FitNodeVal best{.node = _root, .link = &_root, .value = aabb_volume(aabb_merge(fat_aabb, _root->aabb))};
        tree_find_best_fitnode_helper(&best, fat_aabb, 0.0, _root, &_root);

        node->aabb = fat_aabb;

        auto *new_parent = (Node *)malloc(sizeof(Node));
        new_parent->is_self_check = false;
        new_parent->aabb = aabb_merge(best.node->aabb, node->aabb);
        new_parent->parent = best.node->parent;
        new_parent->childs[0] = best.node;
        new_parent->childs[1] = node;
        new_parent->type = new_parent->childs[0]->type & new_parent->childs[1]->type;

        best.node->parent = new_parent;
        node->parent = new_parent;
        *best.link = new_parent;

        auto *ucur_node = new_parent->parent;
        while (ucur_node != nullptr) {
            ucur_node->type = ucur_node->childs[0]->type & ucur_node->childs[1]->type;
            ucur_node->aabb = aabb_merge(ucur_node->childs[0]->aabb, ucur_node->childs[1]->aabb);
            ucur_node = ucur_node->parent;
        }
    }

    void tree_uncheck_selfcheck_flag_helper(Node *cur) {
        cur->is_self_check = false;

        if (tree_is_node_leaf(*cur)) return;
        tree_uncheck_selfcheck_flag_helper(cur->childs[0]);
        tree_uncheck_selfcheck_flag_helper(cur->childs[1]);
    }

    void tree_handle_self_collide_pair(bstc::vector<std::pair<ColliderEntryKey, ColliderEntryKey>> *list, Node *node) {
        if (!node->is_self_check) {
            tree_get_collided_pairs_helper(list, node->childs[0], node->childs[1]);
            node->is_self_check = true;
        }
    }
    void tree_get_collided_pairs_helper(bstc::vector<std::pair<ColliderEntryKey, ColliderEntryKey>> *list, Node *node0, Node *node1) {
        if (node0->type == INERT && node1->type == INERT) return;

        if (tree_is_node_leaf(*node0) && tree_is_node_leaf(*node1)) {
            if (aabb_intersects(get_entry(node0->key)->collider, get_entry(node1->key)->collider)) {
                list->emplace_back(node0->key, node1->key);
            }
            return;
        }

        if (!aabb_intersects(node0->aabb, node1->aabb)) {
            if (!tree_is_node_leaf(*node0) && node0->type != INERT) tree_handle_self_collide_pair(list, node0);
            if (!tree_is_node_leaf(*node1) && node1->type != INERT) tree_handle_self_collide_pair(list, node1);

            return;
        }

        if (tree_is_node_leaf(*node0)) {
            if (node1->type != INERT) tree_handle_self_collide_pair(list, node1);

            tree_get_collided_pairs_helper(list, node0, node1->childs[0]);
            tree_get_collided_pairs_helper(list, node0, node1->childs[1]);

            return;
        }
        if (tree_is_node_leaf(*node1)) {
            if (node0->type != INERT) tree_handle_self_collide_pair(list, node0);

            tree_get_collided_pairs_helper(list, node0->childs[0], node1);
            tree_get_collided_pairs_helper(list, node0->childs[1], node1);

            return;
        }

        if (node0->type != INERT) tree_handle_self_collide_pair(list, node0);
        if (node1->type != INERT) tree_handle_self_collide_pair(list, node1);

        tree_get_collided_pairs_helper(list, node0->childs[0], node1->childs[0]);
        tree_get_collided_pairs_helper(list, node0->childs[0], node1->childs[1]);
        tree_get_collided_pairs_helper(list, node0->childs[1], node1->childs[0]);
        tree_get_collided_pairs_helper(list, node0->childs[1], node1->childs[1]);
    }

    void tree_free_helper(Node *cur) {
        if (cur == nullptr) return;

        if (cur->childs[0] != nullptr) tree_free_helper(cur->childs[0]);
        if (cur->childs[1] != nullptr) tree_free_helper(cur->childs[1]);

        free(cur);
    }
};

} // namespace cactus
