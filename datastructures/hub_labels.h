#pragma once

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "../external/status_log.h"
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

  void removeDuplicateHubs() {
    assert(std::is_sorted(hubs.begin(), hubs.end()));

    std::size_t newSize = 1;
    for (std::size_t i = 1; i < hubs.size(); ++i) {
      if (hubs[newSize - 1] != hubs[i]) {
        hubs[newSize] = hubs[i];
        dists[newSize] = dists[i];
        ++newSize;
      } else {
        dists[newSize - 1] = std::min(dists[newSize - 1], dists[i]);
      }
    }

    hubs.resize(newSize);
    dists.resize(newSize);
  }

  void reserve(std::size_t size) {
    hubs.reserve(size);
    dists.reserve(size);
  }

  std::size_t capacity() const { return hubs.capacity(); }

  std::size_t size() const { return hubs.size(); }

  bool contains(Vertex hub) const {
    return std::find(hubs.begin(), hubs.end(), hub) != hubs.end();
  }

  void add(Vertex hub, Distance dist) {
    assert(hubs.size() == dists.size());
    hubs.emplace_back(hub);
    dists.emplace_back(dist);
  }
};

Distance query(const Label& left, const Label& right) {
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

Distance sub_query(const Label& left, const Label& right, Distance cutoff) {
  Distance result = infinity;
  std::size_t i = 0, j = 0;

  assert(std::is_sorted(left.hubs.begin(), left.hubs.end()));
  assert(std::is_sorted(right.hubs.begin(), right.hubs.end()));

  while (i < left.size() && j < right.size()) {
    if (left.getHub(i) == right.getHub(j)) {
      if (left.getDist(i) < cutoff && right.getDist(j) < cutoff) {
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

Distance sub_query(const Label& left,
                   const std::unordered_map<Vertex, Distance>& lookup,
                   Distance cutoff) {
  Distance result = infinity;

  for (std::size_t i = 0; i < left.size(); ++i) {
    const Vertex hub = left.getHub(i);
    const Distance leftDist = left.getDist(i);

    if (leftDist < cutoff) {
      if (lookup.contains(hub)) {
        const Distance rightDist = lookup.at(hub);
        if (rightDist < cutoff) {
          result =
              std::min(result, static_cast<Distance>(leftDist + rightDist));
        }
      }
    }
  }
  return result;
}

void saveToFile(const std::array<std::vector<Label>, 2>& labels,
                const std::string& fileName) {
  StatusLog log("Save to file");
  std::ofstream outFile(fileName);

  if (!outFile.is_open()) {
    std::cerr << "Error: Unable to open file " << fileName << " for writing.\n";
    return;
  }

  std::size_t N = labels[FWD].size();

  outFile << "V " << N << "\n";

  for (std::size_t v = 0; v < N; ++v) {
    outFile << "o " << v;
    labels[FWD][v].doForAll([&outFile](const Vertex hub, const Distance dist) {
      outFile << " " << (int)hub << " " << (int)dist;
    });
    outFile << "\n";

    outFile << "i " << v;
    labels[BWD][v].doForAll([&outFile](const Vertex hub, const Distance dist) {
      outFile << " " << (int)hub << " " << (int)dist;
    });
    outFile << "\n";
  }

  outFile.close();
}

void benchmark_hublabels(std::array<std::vector<Label>, 2>& labels,
                         const std::size_t numQueries) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;

  assert(labels[FWD].size() == labels[BWD].size());

  std::size_t counter = 0;
  auto queries =
      generateRandomQueries<Vertex>(numQueries, 0, labels[FWD].size());
  long double totalTime(0);
  for (std::pair<Vertex, Vertex> paar : queries) {
    auto t1 = high_resolution_clock::now();
    auto dist = query(labels[FWD][paar.first], labels[BWD][paar.second]);
    auto t2 = high_resolution_clock::now();
    duration<double, std::nano> nano_double = t2 - t1;
    totalTime += nano_double.count();
    counter += (dist != infinity);
  }

  std::cout << "The " << numQueries << " random queries took in total "
            << totalTime << " [ms] and on average "
            << (double)(totalTime / numQueries) << " [ns]! Total of " << counter
            << " of non-infinty results!\n";
}

std::size_t computeTotalBytes(const std::array<std::vector<Label>, 2>& labels) {
  std::size_t totalBytes = 0;

  for (const auto& labelSet : labels) {
    for (const auto& label : labelSet) {
      totalBytes += sizeof(Label);
      totalBytes += label.hubs.capacity() * sizeof(Vertex);
      totalBytes += label.dists.capacity() * sizeof(Distance);
    }
  }

  return totalBytes;
}

void showLabelStats(const std::array<std::vector<Label>, 2>& labels) {
  auto computeStats = [](const std::vector<Label>& currentLabels) {
    std::size_t minSize = std::numeric_limits<std::size_t>::max();
    std::size_t maxSize = 0;
    std::size_t totalSize = 0;

    for (const auto& label : currentLabels) {
      std::size_t size = label.size();
      minSize = std::min(minSize, size);
      maxSize = std::max(maxSize, size);
      totalSize += size;
    }

    double avgSize = static_cast<double>(totalSize) / currentLabels.size();
    return std::make_tuple(minSize, maxSize, avgSize, totalSize);
  };

  auto [inMin, inMax, inAvg, inTotal] = computeStats(labels[BWD]);
  auto [outMin, outMax, outAvg, outTotal] = computeStats(labels[FWD]);

  /* std::locale::global(std::locale("")); */
  /* std::cout.imbue(std::locale()); */

  std::cout << "Forward Labels Statistics:" << std::endl;
  std::cout << "  Min Size:     " << outMin << std::endl;
  std::cout << "  Max Size:     " << outMax << std::endl;
  std::cout << "  Avg Size:     " << outAvg << std::endl;

  std::cout << "Backward Labels Statistics:" << std::endl;
  std::cout << "  Min Size:     " << inMin << std::endl;
  std::cout << "  Max Size:     " << inMax << std::endl;
  std::cout << "  Avg Size:     " << inAvg << std::endl;

  std::cout << "FWD # count:    " << outTotal << std::endl;
  std::cout << "BWD # count:    " << inTotal << std::endl;
  std::cout << "Both # count:   " << (outTotal + inTotal) << std::endl;

  std::cout << "Total memory consumption [megabytes]:" << std::endl;
  std::cout << "  "
            << static_cast<double>(computeTotalBytes(labels) /
                                   (1024.0 * 1024.0))
            << std::endl;
}