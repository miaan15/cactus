module;

#include <algorithm>
#include <boost/container/vector.hpp>
#include <cstdlib>
#include <utility>

export module Physics;

import glm;
import SlotMap;

using namespace glm;
namespace bstc = boost::container;

namespace cactus {

export struct Collider {
    vec2 center;
    vec2 halfexts;
};
export using AABB = mat2x2;

export auto get_aabb(const Collider &a) -> AABB {
    return AABB(a.center - a.halfexts, a.center + a.halfexts);
}

export auto aabb_merge(const AABB &a, const AABB &b) -> AABB {
    return AABB(min(a[0], b[0]), max(a[1], b[1]));
}
export auto aabb_intersects(const AABB &a, const AABB &b) -> bool {
    return all(lessThanEqual(a[0], b[1])) && all(lessThanEqual(b[0], a[1]));
}
export auto aabb_contains(const AABB &a, const AABB &b) -> bool {
    return all(lessThanEqual(a[0], b[0])) && all(lessThanEqual(b[1], a[1]));
}
export auto aabb_volume(const AABB &a) -> float {
    vec2 size = a[1] - a[0];
    return size.x * size.y;
}
export auto aabb_expand_margin(const AABB &a, float margin) -> AABB {
    return AABB(a[0] - vec2(margin, margin), a[1] + vec2(margin, margin));
}
export auto aabb_move(const AABB &a, vec2 d) -> AABB {
    return AABB(a[0] + d, a[1] + d);
}

export struct ColliderEntry {
    Collider coll;
    vec2 vel;

    float invmass;
    float restitution;
    float sfriction;
    float dfriction;

    void *node_ptr;
};
export using ColliderEntrySet = slot_map<ColliderEntry>;
export using ColliderKey = ColliderEntrySet::key_type;

namespace _private {
struct Node {
    Node *parent;
    Node *childs[2];

    ColliderKey key;

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

    auto get(ColliderKey key) -> ColliderEntry * {
        return &entries.at(key);
    }

    auto create(vec2 center = {0, 0}, vec2 halfexts = {.5, .5}, float invmass = 1,
                float restitution = 1, float sfriction = 0, float dfriction = 0) -> ColliderKey {
        auto key = entries.emplace((Collider){center, halfexts}, vec2(0, 0), invmass, restitution,
                                   sfriction, dfriction, nullptr);
        _tree_insert(key);

        return key;
    }

    auto update([[maybe_unused]] float dt) -> void {
        _tree_update();
        _collided_aabbs = _tree_get_collided_pairs();
    }

    auto is_collided(ColliderKey k0, ColliderKey k1) -> bool {
        if (k0.first > k1.first) std::swap(k0, k1);

        auto it = std::lower_bound(_collided_aabbs.begin(), _collided_aabbs.end(),
                                   std::make_pair(k0, k1));

        if (it != _collided_aabbs.end()) {
            return aabb_intersects(get_aabb(get(k0)->coll), get_aabb(get(k1)->coll));
        }
        return false;
    }

    auto resolve_collider(ColliderKey k0, ColliderKey k1) -> void {
        auto e0 = get(k0);
        auto e1 = get(k1);

        vec2 delta = e1->coll.center - e0->coll.center;
        vec2 overlap = (e0->coll.halfexts + e1->coll.halfexts) - abs(delta);

        vec2 normal;
        float penetration;

        if (overlap.x < overlap.y) {
            penetration = overlap.x;
            normal = vec2(delta.x > 0.0f ? 1.0f : -1.0f, 0.0f);
        } else {
            penetration = overlap.y;
            normal = vec2(0.0f, delta.y > 0.0f ? 1.0f : -1.0f);
        }

        vec2 relVel = e1->vel - e0->vel;
        float velAlongNormal = dot(relVel, normal);

        if (velAlongNormal > 0.0f) {
            return;
        }

        float restitution = std::min(e0->restitution, e1->restitution);

        float j = -(1.0f + restitution) * velAlongNormal;
        j /= e0->invmass + e1->invmass;

        vec2 impulse = j * normal;
        e0->vel -= e0->invmass * impulse;
        e1->vel += e1->invmass * impulse;

        const float percent = 0.8f;
        const float slop = 0.01f;
        vec2 correction =
            normal * (std::max(penetration - slop, 0.0f) / (e0->invmass + e1->invmass)) * percent;

        e0->coll.center -= e0->invmass * correction;
        e1->coll.center += e1->invmass * correction;

        relVel = e1->vel - e0->vel;

        vec2 tangent = relVel - dot(relVel, normal) * normal;
        float tangentLen = length(tangent);

        if (tangentLen > 0.0001f) {
            tangent /= tangentLen;

            float jt = -dot(relVel, tangent);
            jt /= e0->invmass + e1->invmass;

            float mu = length(vec2(e0->sfriction, e1->sfriction));
            vec2 frictionImpulse;

            if (std::abs(jt) < j * mu) {
                frictionImpulse = jt * tangent;
            } else {
                float dmu = length(vec2(e0->dfriction, e1->dfriction));
                frictionImpulse = -j * tangent * dmu;
            }

            e0->vel -= e0->invmass * frictionImpulse;
            e1->vel += e1->invmass * frictionImpulse;
        }
    }

    ~PhysicsWorld() {
        _tree_free_helper(_root);
    }

    auto get_entry_aabb(ColliderKey key, float dt) -> AABB {
        auto entry_aabb = get_aabb(get(key)->coll);
        auto entry_vel = get(key)->vel;
        return aabb_merge(entry_aabb, aabb_move(entry_aabb, entry_vel * dt));
    }

    // AABBTree
    bstc::vector<std::pair<ColliderKey, ColliderKey>> _collided_aabbs;

    Node *_root;

    auto _tree_insert(ColliderKey key, uint8_t flag = 0) -> ColliderKey {
        if (_root == nullptr) {
            auto *node = (Node *)malloc(sizeof(Node));
            get(key)->node_ptr = node;
            *node = {.key = key,
                     .aabb = aabb_expand_margin(get_aabb(get(key)->coll), margin),
                     .flag = flag};

            _root = node;

            return key;
        }

        auto entry_aabb = get_aabb(get(key)->coll);
        AABB fat_aabb = aabb_expand_margin(entry_aabb, margin);

        FitNodeVal best{
            .node = _root, .link = &_root, .value = aabb_volume(aabb_merge(fat_aabb, _root->aabb))};
        _tree_find_best_fitnode_helper(&best, fat_aabb, 0.0, _root, &_root);

        auto *node = (Node *)malloc(sizeof(Node));
        get(key)->node_ptr = node;
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

    auto _tree_remove(ColliderKey key) -> bool {
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
            auto entry_collider = get_aabb(get(_root->key)->coll);
            if (!aabb_contains(_root->aabb, entry_collider)) {
                _root->aabb = aabb_expand_margin(entry_collider, margin);
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

    auto _tree_get_collided_pairs() -> bstc::vector<std::pair<ColliderKey, ColliderKey>> {
        bstc::vector<std::pair<ColliderKey, ColliderKey>> res{};
        if (_root == nullptr) return res;
        if (_tree_is_node_leaf(*_root)) return res;

        _tree_uncheck_selfcheck_flag_helper(_root);
        _tree_get_collided_pairs_helper(&res, _root->childs[0], _root->childs[1]);

        std::sort(res.begin(), res.end());
        return res;
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

    bool _tree_remove_helper(ColliderKey entry_key, Node *cur, Node **cur_link) {
        bool res = false;
        if (!_tree_is_node_leaf(*cur->childs[0])) {
            if (aabb_contains(cur->childs[0]->aabb, get_aabb(get(entry_key)->coll))) {
                res |= _tree_remove_helper(entry_key, cur->childs[0], &cur->childs[0]);
            }
        } else {
            if (cur->childs[0]->key == entry_key) {
                *cur_link = cur->childs[1];
                cur->childs[1]->parent = cur->parent;
                free(cur->childs[0]);
                free(cur);
                return true;
            }
        }

        if (!_tree_is_node_leaf(*cur->childs[1])) {
            if (aabb_contains(cur->childs[1]->aabb, get_aabb(get(entry_key)->coll))) {
                res |= _tree_remove_helper(entry_key, cur->childs[1], &cur->childs[1]);
            }
        } else {
            if (cur->childs[1]->key == entry_key) {
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
            if (!aabb_contains(cur->aabb, get_aabb(get(cur->key)->coll))) {
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

        auto entry_aabb = get_aabb(get(node->key)->coll);
        AABB fat_aabb = aabb_expand_margin(entry_aabb, margin);

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

    void _tree_handle_self_collide_pair(bstc::vector<std::pair<ColliderKey, ColliderKey>> *list,
                                        Node *node) {
        if (!node->is_self_check) {
            _tree_get_collided_pairs_helper(list, node->childs[0], node->childs[1]);
            node->is_self_check = true;
        }
    }
    void _tree_get_collided_pairs_helper(bstc::vector<std::pair<ColliderKey, ColliderKey>> *list,
                                         Node *node0, Node *node1) {
        if ((node0->flag & node1->flag) != 0) return;

        if (_tree_is_node_leaf(*node0) && _tree_is_node_leaf(*node1)) {
            if (aabb_intersects(get_aabb(get(node0->key)->coll), get_aabb(get(node1->key)->coll))) {
                if (node0->key.first > node1->key.second) std::swap(node0->key, node1->key);
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
