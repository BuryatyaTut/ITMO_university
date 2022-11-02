#pragma once

#include <functional>
#include <random>
#include <utility>

template <typename Left, typename Right, typename compare_left,
          typename compare_right>
struct bimap;

namespace tree_inside {

thread_local inline std::mt19937 rnd{};

struct left_tag;
struct right_tag;

struct elem_base {
  elem_base() noexcept = default;

  bool is_left_son() noexcept {
    return parent->left == this;
  }

  bool is_right_son() noexcept {
    return parent->right == this;
  }

  void change_parent(elem_base* new_parent) noexcept {
    parent = new_parent;
  }

  template <typename Left, typename Right, typename compare_left,
            typename compare_right>
  friend struct ::bimap;

  template <typename T, typename Tag, typename Comp>
  friend struct treap;

private:
  elem_base* parent{nullptr};
  elem_base* left{nullptr};
  elem_base* right{nullptr};
};

template <typename T, typename Tag>
struct elem : elem_base {
  elem() noexcept = default;

  explicit elem(T val_) noexcept : val(std::move(val_)), prior(rnd()) {}

  ~elem() = default;

  elem(elem const&) = delete;
  elem& operator=(elem const&) = delete;

  template <typename Left, typename Right, typename compare_left,
            typename compare_right>
  friend struct ::bimap;

  template <typename T_, typename Tag_, typename Comp_>
  friend struct treap;

private:
  T val;
  uint32_t prior{static_cast<uint32_t>(-1)};
};

template <typename Key, typename Value>
struct bimap_node : elem<Key, left_tag>, elem<Value, right_tag> {
  bimap_node() noexcept = default;

  template <typename Key_, typename Value_>
  bimap_node(Key_&& left, Value_&& right) noexcept
      : elem<Key, left_tag>(std::forward<Key_>(left)), elem<Value, right_tag>(
                                                           std::forward<Value_>(
                                                               right)) {}
};

template <typename T, typename Tag, typename comp>
struct treap : comp {
  using treap_element_t = elem<T, Tag>;

  treap() noexcept = default;

  treap(const treap&) = delete;
  explicit treap(const comp& cmp_) : comp(cmp_) {}

  treap(treap&& other) noexcept {
    swap(other);
  }
  explicit treap(comp&& cmp_) noexcept : comp(std::move(cmp_)) {}

  treap& operator=(const treap&) = delete;
  treap& operator=(treap&&) = delete;

  ~treap() = default;

  bool empty() const noexcept {
    return fake.left == nullptr;
  }

  elem_base* insert(treap_element_t& node) noexcept {
    if (!empty()) {
      auto [left, right] = split(node.val, get_treap_elem(fake.left));
      treap_element_t* pointer = merge(left, &node);
      fake.left = merge(pointer, right);
    } else {
      fake.left = &get_base_elem(node);
    }

    fake.left->parent = &fake;
    return &node;
  }

  void erase(elem_base* obj_elem) noexcept {
    treap_element_t* res =
        merge(get_treap_elem(obj_elem->left), get_treap_elem(obj_elem->right));

    if (res != nullptr) {
      res->change_parent(obj_elem->parent);
    }
    if (obj_elem == fake.left) {
      fake.left = res;
    }

    if (!obj_elem->is_right_son()) {
      obj_elem->parent->left = res;
    } else {
      obj_elem->parent->right = res;
    }

    obj_elem->left = obj_elem->right = obj_elem->parent = nullptr;
  }

  bool erase_in_subtree(T const& val, treap_element_t* obj_elem) noexcept {
    if (obj_elem == nullptr) {
      return false;
    }

    if (more_or_equal(obj_elem->val, val)) {
      erase(obj_elem);
      return true;
    } else if (less(obj_elem->val, val)) {
      return erase_in_subtree(val, obj_elem->right);
    }
    return erase_in_subtree(val, obj_elem->left);
  }

  bool erase(T const& val) noexcept {
    return erase_in_subtree(val, get_treap_elem(fake.left));
  }

  elem_base const* min() const noexcept {
    return min(&fake);
  }

  treap_element_t* find(T const& val) const noexcept {
    return find(val, get_treap_elem(fake.left));
  }

  elem_base const* upper_bound(const T& x) const noexcept {
    return bound(
        x, [this](const T& a, const T& b) { return more_or_equal(a, b); });
  }

  elem_base const* lower_bound(const T& x) const noexcept {
    return bound(x, [this](const T& a, const T& b) { return compare(b, a); });
  }

  bool more_or_equal(const T& x, const T& y) const noexcept {
    return !compare(x, y);
  }

  bool less(const T& x, const T& y) const noexcept {
    return compare(x, y);
  }

  bool equal(const T& x, const T& y) const noexcept {
    return !compare(x, y) && more_or_equal(x, y);
  }

  template <typename Left, typename Right, typename compare_left,
            typename compare_right>
  friend struct ::bimap;

  void swap(treap& other) noexcept {
    swap_fake(fake, other.fake);
    std::swap(get_cmp(), other.get_cmp());
  }

  static void swap_fake(elem_base& x, elem_base& y) noexcept {
    std::swap(x.left, y.left);
    std::swap(x.right, y.right);
    std::swap(x.right->right, y.right->right);

    if (y.left != nullptr) {
      y.left->parent = &y;
    }

    if (x.left != nullptr) {
      x.left->parent = &x;
    }
  }

private:
  elem_base fake;

  comp& get_cmp() {
    return static_cast<comp&>(*this);
  }

  comp const& get_cmp() const {
    return static_cast<comp const&>(*this);
  }

  bool compare(const T& x, const T& y) const noexcept {
    return get_cmp()(x, y);
  }

  static elem_base& get_base_elem(treap_element_t& elem) noexcept {
    return static_cast<elem_base&>(elem);
  }

  static treap_element_t* get_treap_elem(elem_base* base) noexcept {
    return static_cast<treap_element_t*>(base);
  }

  template <typename F>
  elem_base const* bound(const T& x, F&& check_bound) const noexcept {
    elem_base const* elem = &fake;
    treap_element_t const* curr_elem = get_treap_elem(fake.left);

    while (curr_elem != nullptr) {
      if (!check_bound(x, curr_elem->val)) {
        elem = curr_elem;
        curr_elem = get_treap_elem(curr_elem->left);
      } else {
        curr_elem = get_treap_elem(curr_elem->right);
      }
    }

    return elem;
  }

  treap_element_t* find(T const& val, treap_element_t* node) const noexcept {
    if (node == nullptr) {
      return nullptr;
    }

    if (less(node->val, val)) {
      return find(val, get_treap_elem(node->right));
    } else if (more_or_equal(val, node->val)) {
      return node;
    } else {
      return find(val, get_treap_elem(node->left));
    }
  }

  static elem_base const* min(elem_base const* obj_elem) noexcept {
    elem_base const* pointer = obj_elem;
    while (pointer->left != nullptr) {
      pointer = pointer->left;
    }
    return pointer;
  }

  treap_element_t* merge(treap_element_t* first,
                         treap_element_t* second) noexcept {
    if (first == nullptr) {
      return second;
    }

    if (second == nullptr) {
      return first;
    }

    if (first->prior < second->prior) {
      elem_base* res = merge(first, get_treap_elem(second->left));
      second->left = res;
      if (res != nullptr) {
        res->change_parent(second);
      }
      return second;
    } else {
      elem_base* res = merge(get_treap_elem(first->right), second);
      first->right = res;
      if (res != nullptr) {
        res->change_parent(first);
      }
      return first;
    }
  }

  std::pair<treap_element_t*, treap_element_t*>
  split(T& x, treap_element_t* obj_elem) noexcept {
    if (obj_elem == nullptr) {
      return {nullptr, nullptr};
    }

    if (less(obj_elem->val, x)) {
      std::pair<elem_base*, elem_base*> split_obj_elems =
          split(x, get_treap_elem(obj_elem->right));
      obj_elem->right = split_obj_elems.first;

      if (split_obj_elems.first != nullptr) {
        split_obj_elems.first->change_parent(obj_elem);
      }

      if (split_obj_elems.second != nullptr) {
        split_obj_elems.second->change_parent(nullptr);
      }

      return {obj_elem, get_treap_elem(split_obj_elems.second)};
    } else {
      std::pair<elem_base*, elem_base*> split_obj_elems =
          split(x, get_treap_elem(obj_elem->left));
      obj_elem->left = split_obj_elems.second;

      if (split_obj_elems.second != nullptr) {
        split_obj_elems.second->change_parent(obj_elem);
      }

      if (split_obj_elems.first != nullptr) {
        split_obj_elems.first->change_parent(nullptr);
      }

      return {get_treap_elem(split_obj_elems.first), obj_elem};
    }
  }
};
} // namespace tree_inside
