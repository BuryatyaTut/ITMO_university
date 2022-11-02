#pragma once

#include "treap.h"
#include <cstdint>
#include <stdexcept>
#include <utility>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
  using left_t = Left;
  using right_t = Right;

  using elem_base = tree_inside::elem_base;
  using left_tag = tree_inside::left_tag;
  using right_tag = tree_inside::right_tag;

  using node_t = tree_inside::bimap_node<Left, Right>;
  using left_node_t = tree_inside::elem<Left, tree_inside::left_tag>;
  using right_node_t = tree_inside::elem<Right, tree_inside::right_tag>;
  using left_treap_t =
      tree_inside::treap<left_t, tree_inside::left_tag, CompareLeft>;
  using right_treap_t =
      tree_inside::treap<right_t, tree_inside::right_tag, CompareRight>;

  template <typename value, typename Tag>
  struct iterator {
    iterator() = delete;

    explicit iterator(const elem_base* ptr_) noexcept
        : elem_value(const_cast<elem_base*>(ptr_)) {}

    value const* operator->() const noexcept {
      return &**this;
    }

    value const& operator*() const noexcept {
      using to = std::conditional_t<std::is_same_v<Tag, left_tag>, left_node_t,
                                    right_node_t>;
      return static_cast<to*>(elem_value)->val;
    }

    iterator operator--(int) noexcept {
      iterator res = iterator(elem_value);

      --(*this);
      return res;
    }

    iterator& operator--() noexcept {
      if (elem_value->left) {
        elem_value = elem_value->left;
        while (elem_value->right) {
          elem_value = elem_value->right;
        }
      } else if (elem_value->is_right_son()) {
        elem_value = elem_value->parent;
      } else {
        while (elem_value->is_left_son()) {
          elem_value = elem_value->parent;
        }
        elem_value = elem_value->parent;
      }
      return *this;
    }

    iterator operator++(int) noexcept {
      iterator res = iterator(elem_value);
      ++(*this);
      return res;
    }

    iterator& operator++() noexcept {
      if (elem_value->right) {
        elem_value = elem_value->right;
        while (elem_value->left) {
          elem_value = elem_value->left;
        }
      } else if (elem_value->is_left_son()) {
        elem_value = elem_value->parent;
      } else if (elem_value->is_right_son()) {
        while (elem_value->is_right_son()) {
          elem_value = elem_value->parent;
        }
        elem_value = elem_value->parent;
      }
      return *this;
    }

    using tag =
        std::conditional_t<std::is_same_v<Tag, left_tag>, right_tag, left_tag>;
    using t =
        std::conditional_t<std::is_same_v<Tag, left_tag>, right_t, left_t>;
    using curr_it = iterator<t, tag>;

    curr_it flip() const noexcept {
      elem_base* ptr;

      if (elem_value->parent) {
        using from = std::conditional_t<std::is_same_v<Tag, left_tag>,
                                        left_node_t, right_node_t>;
        using to = std::conditional_t<std::is_same_v<Tag, right_tag>,
                                      left_node_t, right_node_t>;
        ptr = static_cast<to*>(
            static_cast<node_t*>(static_cast<from*>(elem_value)));
      } else {
        ptr = elem_value->right;
      }
      return curr_it(ptr);
    }

    bool operator==(iterator const& other) const noexcept {
      return elem_value == other.elem_value;
    }

    bool operator!=(iterator const& other) const noexcept {
      return !(*this == other);
    }

    friend bimap;

  private:
    elem_base* elem_value;
  };

  using left_iterator = iterator<left_t, left_tag>;
  using right_iterator = iterator<right_t, right_tag>;

  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight()) noexcept
      : right_treap(std::move(compare_right)),
        left_treap(std::move(compare_left)) {

    right_treap.fake.right = &left_treap.fake;
    left_treap.fake.right = &right_treap.fake;
  }

  bimap(bimap const& other)
      : right_treap(static_cast<CompareRight const&>(other.right_treap)),
        left_treap(static_cast<CompareLeft const&>(other.left_treap)) {
    right_treap.fake.right = &left_treap.fake;
    left_treap.fake.right = &right_treap.fake;

    for (left_iterator left_it = other.begin_left();
         left_it != other.end_left(); ++left_it) {
      insert(*left_it, *left_it.flip());
    }
  }

  bimap(bimap&& other) noexcept
      : right_treap(std::move(other.right_treap)), size_(other.size_),
        left_treap(std::move(other.left_treap)) {}

  bimap& operator=(bimap const& other) {
    if (this != &other) {
      bimap tmp(other);
      swap(tmp);
    }

    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (this != &other) {
      swap(other);
      other.erase_left(begin_left(), end_left());
    }
    return *this;
  }

  ~bimap() {
    erase_left(begin_left(), end_left());
  }

  void swap(bimap& other) noexcept {

    right_treap.swap(other.right_treap);
    left_treap.swap(other.left_treap);

    std::swap(size_, other.size_);
  }

  template <typename left_t_ = left_t, typename right_t_ = right_t>
  left_iterator insert(left_t_&& left, right_t_&& right) {
    if (right_treap.find(right) != nullptr) {
      return end_left();
    }

    if (left_treap.find(left) != nullptr) {
      return end_left();
    }

    auto* curr_node =
        new node_t(std::forward<left_t_>(left), std::forward<right_t_>(right));
    elem_base* l_ptr = left_treap.insert(*curr_node);
    right_treap.insert(*curr_node);
    size_++;
    return left_iterator(l_ptr);
  }

  left_iterator erase_left(left_iterator it) noexcept {
    left_iterator copy(it.elem_value);
    ++copy;

    node_t* ptr = left_base_double(it.elem_value);

    right_treap.erase(node_right(ptr));
    left_treap.erase(it.elem_value);

    size_--;
    delete ptr;

    return copy;
  }

  bool erase_left(left_t const& left) noexcept {
    left_iterator it = find_left(left);

    if (it != end_left()) {
      erase_left(it);
      return true;
    }
    return false;
  }

  right_iterator erase_right(right_iterator it) noexcept {
    right_iterator copy(it.elem_value);
    ++copy;

    node_t* pointer = right_base_double(it.elem_value);

    right_treap.erase(it.elem_value);
    left_treap.erase(node_left(pointer));

    size_--;
    delete pointer;

    return copy;
  }

  bool erase_right(right_t const& right) noexcept {
    right_iterator it = find_right(right);

    if (it != end_right()) {
      erase_right(it);
      return true;
    }
    return false;
  }

  left_iterator erase_left(left_iterator first, left_iterator last) noexcept {
    while (first != last) {
      erase_left(first++);
    }
    return first;
  }

  right_iterator erase_right(right_iterator first,
                             right_iterator last) noexcept {
    while (first != last) {
      erase_right(first++);
    }
    return first;
  }

  left_iterator find_left(left_t const& left) const noexcept {
    elem_base* ptr = left_treap.find(left);
    return ptr != nullptr ? left_iterator(ptr) : end_left();
  }

  right_iterator find_right(right_t const& right) const noexcept {
    elem_base* ptr = right_treap.find(right);
    return ptr != nullptr ? right_iterator(ptr) : end_right();
  }

  right_t const& at_left(left_t const& key) const {
    left_iterator it = find_left(key);

    if (it == end_left())
      throw std::out_of_range("no such element");

    return *it.flip();
  }

  left_t const& at_right(right_t const& key) const {
    right_iterator it = find_right(key);

    if (it == end_right())
      throw std::out_of_range("no such element");
    return *it.flip();
  }

  template <typename = std::enable_if<std::is_default_constructible_v<Right>>>
  right_t const& at_left_or_default(left_t const& key) noexcept {
    left_iterator left_it = find_left(key);

    if (left_it == end_left()) {
      right_t default_right = right_t();
      right_iterator right_it = find_right(default_right);

      if (right_it == end_right())
        return *insert(key, std::move(default_right)).flip();

      left_iterator left_it_2 = right_it.flip();
      left_treap.erase(left_it_2.elem_value);
      left_base(left_it_2.elem_value)->val = key;
      left_treap.insert(*left_base(left_it_2.elem_value));
      return *right_it;
    }
    return *left_it.flip();
  }

  template <typename = std::enable_if<std::is_default_constructible_v<Left>>>
  left_t const& at_right_or_default(right_t const& key) noexcept {
    right_iterator right_it = find_right(key);

    if (right_it == end_right()) {
      left_t default_left = left_t();
      left_iterator left_it = find_left(default_left);

      if (left_it == end_left())
        return *insert(std::move(default_left), key);

      right_iterator rit1 = left_it.flip();
      right_treap.erase(rit1.elem_value);
      right_base(rit1.elem_value)->val = key;
      right_treap.insert(*right_base(rit1.elem_value));
      return *left_it;
    }
    return *right_it.flip();
  }

  left_iterator lower_bound_left(const left_t& left) const noexcept {
    return left_iterator(left_treap.lower_bound(left));
  }
  left_iterator upper_bound_left(const left_t& left) const noexcept {
    return left_iterator(left_treap.upper_bound(left));
  }

  right_iterator lower_bound_right(const right_t& right) const noexcept {
    return right_iterator(right_treap.lower_bound(right));
  }

  right_iterator upper_bound_right(const right_t& right) const noexcept {
    return right_iterator(right_treap.upper_bound(right));
  }

  left_iterator begin_left() const noexcept {
    return left_iterator(left_treap.min());
  }

  left_iterator end_left() const noexcept {
    return left_iterator(&left_treap.fake);
  }

  right_iterator begin_right() const noexcept {
    return right_iterator(right_treap.min());
  }

  right_iterator end_right() const noexcept {
    return right_iterator(&right_treap.fake);
  }

  bool empty() const noexcept {
    return size_ == 0;
  }

  std::size_t size() const noexcept {
    return size_;
  }

  friend bool operator==(bimap const& a, bimap const& b) noexcept {
    if (a.size_ != b.size_) {
      return false;
    }

    left_iterator left_it_a = a.begin_left();
    left_iterator left_it_b = b.begin_left();
    left_iterator a_end = a.end_left();

    while (left_it_a != a_end) {
      if (!(a.right_treap.equal(*left_it_a.flip(), *left_it_b.flip()) &&
            a.left_treap.equal(*left_it_a, *left_it_b))) {
        return false;
      }
      left_it_a++;
      left_it_b++;
    }

    return true;
  }
  friend bool operator!=(bimap const& a, bimap const& b) noexcept {
    return !(a == b);
  }

private:
  size_t size_{0};
  left_treap_t left_treap;
  right_treap_t right_treap;

  static node_t* left_base_double(elem_base* left) noexcept {
    return static_cast<node_t*>(static_cast<left_node_t*>(left));
  }

  static node_t* right_base_double(elem_base* right) noexcept {
    return static_cast<node_t*>(static_cast<right_node_t*>(right));
  }

  static left_node_t* left_base(elem_base* left) noexcept {
    return static_cast<left_node_t*>(left);
  }

  static right_node_t* right_base(elem_base* right) noexcept {
    return static_cast<right_node_t*>(right);
  }

  static right_node_t* node_right(node_t* curr_node) noexcept {
    return static_cast<right_node_t*>(curr_node);
  }

  static left_node_t* node_left(node_t* curr_node) noexcept {
    return static_cast<left_node_t*>(curr_node);
  }
};
