#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <vector>

#include "types.h"
#include "utils.h"

struct Label {
  std::vector<Vertex> hubs;
  std::vector<Distance> dists;

  Label() = default;
  Label(const Label&) = default;
  Label(Label&&) noexcept = default;
  Label& operator=(const Label&) = default;
  Label& operator=(Label&&) noexcept = default;

  void clear() {
    hubs.clear();
    dists.clear();
    hubs.shrink_to_fit();
    dists.shrink_to_fit();
  }

  [[nodiscard]] Vertex getHub(std::size_t i) const {
    assert(i < hubs.size());
    return hubs[i];
  }

  [[nodiscard]] Distance getDist(std::size_t i) const {
    assert(i < dists.size());
    return dists[i];
  }

  template <typename FUNC>
  void doForAll(FUNC&& apply) const {
    assert(hubs.size() == dists.size());
    for (std::size_t i = 0; i < hubs.size(); ++i) {
      apply(hubs[i], dists[i]);
    }
  }

  void print() const {
    doForAll([](Vertex h, Distance d) {
      std::cout << "Hub: " << h << ", Dist: " << static_cast<int>(d) << '\n';
    });
  }

  void sort() {
    assert(hubs.size() == dists.size());
    std::vector<std::size_t> p = sort_permutation(
        hubs, [](Vertex left, Vertex right) { return left < right; });

    apply_permutation_in_place(hubs, p);
    apply_permutation_in_place(dists, p);
  }

  void reserve(std::size_t size) {
    hubs.reserve(size);
    dists.reserve(size);
  }

  std::size_t capacity() const { return hubs.capacity(); }

  [[nodiscard]] std::size_t size() const { return hubs.size(); }

  [[nodiscard]] bool contains(Vertex hub) const {
    return std::find(hubs.begin(), hubs.end(), hub) != hubs.end();
  }

  void add(Vertex hub, Distance dist) {
    assert(hubs.size() == dists.size());
    hubs.emplace_back(hub);
    dists.emplace_back(dist);
  }
};

[[nodiscard]] Distance query(const Label& left, const Label& right) {
  Distance result = infinity;
  std::size_t i = 0, j = 0;

  assert(std::is_sorted(left.hubs.begin(), left.hubs.end()));
  assert(std::is_sorted(right.hubs.begin(), right.hubs.end()));

  while (i < left.size() && j < right.size()) {
    if (left.getHub(i) == right.getHub(j)) {
      result = std::min(
          result, static_cast<Distance>(left.getDist(i) + right.getDist(j)));
      ++i;
      ++j;
    } else if (left.getHub(i) < right.getHub(j)) {
      ++i;
    } else {
      ++j;
    }
  }
  return result;
}

[[nodiscard]] Distance sub_query(const Label& left, const Label& right,
                                 Distance dist) {
  Distance result = infinity;
  std::size_t i = 0, j = 0;

  assert(std::is_sorted(left.hubs.begin(), left.hubs.end()));
  assert(std::is_sorted(right.hubs.begin(), right.hubs.end()));

  while (i < left.size() && j < right.size()) {
    if (left.getHub(i) == right.getHub(j)) {
      if (left.getDist(i) < dist && right.getDist(j) < dist) {
        result = std::min(
            result, static_cast<Distance>(left.getDist(i) + right.getDist(j)));
      }
      ++i;
      ++j;
    } else if (left.getHub(i) < right.getHub(j)) {
      ++i;
    } else {
      ++j;
    }
  }
  return result;
}
