#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <fstream>
#include <iostream>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "../external/status_log.h"
#include "spin_lock.h"
#include "types.h"
#include "utils.h"

struct Label {
  std::vector<Vertex> hubs;
  std::vector<Distance> dists;
  mutable Spinlock lock;

  Label() = default;
  Label(const Label& other) {
    std::lock_guard<Spinlock> guard(other.lock);
    hubs = other.hubs;
    dists = other.dists;
  }

  void clear() {
    std::lock_guard<Spinlock> guard(lock);

    hubs.clear();
    dists.clear();
    hubs.shrink_to_fit();
    dists.shrink_to_fit();
  }

  [[nodiscard]] Vertex getHub(std::size_t i) const {
    std::lock_guard<Spinlock> guard(lock);

    assert(i < hubs.size());
    return hubs[i];
  }

  [[nodiscard]] Distance getDist(std::size_t i) const {
    std::lock_guard<Spinlock> guard(lock);

    assert(i < dists.size());
    return dists[i];
  }

  template <typename FUNC>
  void doForAll(FUNC&& apply) const {
    std::lock_guard<Spinlock> guard(lock);

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
    std::lock_guard<Spinlock> guard(lock);

    assert(hubs.size() == dists.size());
    std::vector<std::size_t> p = sort_permutation(
        hubs, [](Vertex left, Vertex right) { return left < right; });

    apply_permutation_in_place(hubs, p);
    apply_permutation_in_place(dists, p);
  }

  void removeDuplicateHubs() {
    std::lock_guard<Spinlock> guard(lock);

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
    std::lock_guard<Spinlock> guard(lock);

    hubs.reserve(size);
    dists.reserve(size);
  }

  std::size_t capacity() const {
    std::lock_guard<Spinlock> guard(lock);
    return hubs.capacity();
  }

  std::size_t size() const {
    std::lock_guard<Spinlock> guard(lock);
    return hubs.size();
  }

  bool contains(Vertex hub) const {
    std::lock_guard<Spinlock> guard(lock);

    return std::find(hubs.begin(), hubs.end(), hub) != hubs.end();
  }

  void add(Vertex hub, Distance dist) {
    std::lock_guard<Spinlock> guard(lock);

    assert(hubs.size() == dists.size());
    hubs.emplace_back(hub);
    dists.emplace_back(dist);
  }
};

template <typename TYPE_BITSET = std::uint8_t>
struct BitParallelLabels {
  std::vector<Vertex> hubs;
  std::vector<Distance> dists;
  std::array<std::vector<TYPE_BITSET>, 2> bitsets_s;

  mutable Spinlock lock;

  BitParallelLabels() = default;
  BitParallelLabels(const BitParallelLabels& other) {
    std::lock_guard<Spinlock> guard(other.lock);
    hubs = other.hubs;
    dists = other.dists;
    bitsets_s[1] = other.bitsets_s[1];
    bitsets_s[0] = other.bitsets_s[0];
  }

  void clear() {
    std::lock_guard<Spinlock> guard(lock);

    hubs.clear();
    dists.clear();
    bitsets_s[1].clear();
    bitsets_s[0].clear();

    hubs.shrink_to_fit();
    dists.shrink_to_fit();
    bitsets_s[1].shrink_to_fit();
    bitsets_s[0].shrink_to_fit();
  }

  [[nodiscard]] Vertex getHub(std::size_t i) const {
    std::lock_guard<Spinlock> guard(lock);

    assert(i < hubs.size());
    return hubs[i];
  }

  [[nodiscard]] Distance getDist(std::size_t i) const {
    std::lock_guard<Spinlock> guard(lock);

    assert(i < dists.size());
    return dists[i];
  }

  [[nodiscard]] TYPE_BITSET getBitSetS_1(std::size_t i) const {
    std::lock_guard<Spinlock> guard(lock);

    assert(i < bitsets_s[1].size());
    return bitsets_s[1][i];
  }

  [[nodiscard]] TYPE_BITSET getBitSetS_0(std::size_t i) const {
    std::lock_guard<Spinlock> guard(lock);

    assert(i < bitsets_s[0].size());
    return bitsets_s[0][i];
  }

  void orBitsetS_1(std::size_t i, TYPE_BITSET otherS_1) {
    std::lock_guard<Spinlock> guard(lock);

    assert(i < bitsets_s[1].size());
    bitsets_s[1][i] |= otherS_1;
  }

  void orBitsetS_0(std::size_t i, TYPE_BITSET otherS_0) {
    std::lock_guard<Spinlock> guard(lock);

    assert(i < bitsets_s[0].size());
    bitsets_s[0][i] |= otherS_0;
  }

  template <typename FUNC>
  void doForAll(FUNC&& apply) const {
    std::lock_guard<Spinlock> guard(lock);

    assert(hubs.size() == dists.size());
    assert(hubs.size() == bitsets_s[1].size());
    assert(hubs.size() == bitsets_s[0].size());

    for (std::size_t i = 0; i < hubs.size(); ++i) {
      apply(hubs[i], dists[i], bitsets_s[1][i], bitsets_s[0][i]);
    }
  }

  void print() const {
    doForAll([](Vertex h, Distance d, TYPE_BITSET s_1, TYPE_BITSET s_0) {
      std::cout << "Hub: " << h << ", Dist: " << static_cast<int>(d)
                << ", S_{-1}: " << std::bitset<(sizeof(TYPE_BITSET) << 3)>(s_1)
                << ", S_{0}: " << std::bitset<(sizeof(TYPE_BITSET) << 3)>(s_0)
                << "\n";
    });
  }

  void sort() {
    std::lock_guard<Spinlock> guard(lock);

    assert(hubs.size() == dists.size());
    assert(hubs.size() == bitsets_s[1].size());
    assert(hubs.size() == bitsets_s[0].size());

    std::vector<std::size_t> p = sort_permutation(
        hubs, [](Vertex left, Vertex right) { return left < right; });

    apply_permutation_in_place(hubs, p);
    apply_permutation_in_place(dists, p);
    apply_permutation_in_place(bitsets_s[1], p);
    apply_permutation_in_place(bitsets_s[0], p);
  }

  void removeDuplicateHubs() {
    std::lock_guard<Spinlock> guard(lock);

    assert(std::is_sorted(hubs.begin(), hubs.end()));

    std::size_t newSize = 1;
    for (std::size_t i = 1; i < hubs.size(); ++i) {
      if (hubs[newSize - 1] != hubs[i]) {
        hubs[newSize] = hubs[i];
        dists[newSize] = dists[i];
        bitsets_s[1][newSize] = bitsets_s[1][i];
        bitsets_s[0][newSize] = bitsets_s[0][i];
        ++newSize;
      } else {
        dists[newSize - 1] = std::min(dists[newSize - 1], dists[i]);
      }
    }

    hubs.resize(newSize);
    dists.resize(newSize);
    bitsets_s[1].resize(newSize);
    bitsets_s[0].resize(newSize);
  }

  void reserve(std::size_t size) {
    std::lock_guard<Spinlock> guard(lock);

    hubs.reserve(size);
    dists.reserve(size);
    bitsets_s[1].reserve(size);
    bitsets_s[0].reserve(size);
  }

  std::size_t capacity() const {
    std::lock_guard<Spinlock> guard(lock);
    return hubs.capacity();
  }

  std::size_t size() const {
    std::lock_guard<Spinlock> guard(lock);
    return hubs.size();
  }

  bool contains(Vertex hub) const {
    std::lock_guard<Spinlock> guard(lock);

    return std::find(hubs.begin(), hubs.end(), hub) != hubs.end();
  }

  void add(Vertex hub, Distance dist, TYPE_BITSET s_1, TYPE_BITSET s_0) {
    std::lock_guard<Spinlock> guard(lock);

    assert(hubs.size() == dists.size());
    assert(hubs.size() == bitsets_s[1].size());
    assert(hubs.size() == bitsets_s[0].size());

    hubs.emplace_back(hub);
    dists.emplace_back(dist);
    bitsets_s[1].emplace_back(s_1);
    bitsets_s[0].emplace_back(s_0);
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

template <typename TYPE_BITSET>
Distance query(const BitParallelLabels<TYPE_BITSET>& left,
               const BitParallelLabels<TYPE_BITSET>& right) {
  Distance result = infinity;
  std::size_t i = 0, j = 0;

  assert(std::is_sorted(left.hubs.begin(), left.hubs.end()));
  assert(std::is_sorted(right.hubs.begin(), right.hubs.end()));

  while (i < left.size() && j < right.size()) {
    if (left.getHub(i) == right.getHub(j)) {
      Distance new_distance = left.getDist(i) + right.getDist(j);

      new_distance -= ((left.getBitSetS_1(i) & right.getBitSetS_1(j)) ? 2
                       : ((left.getBitSetS_0(i) & right.getBitSetS_1(j)) |
                          (left.getBitSetS_1(i) & right.getBitSetS_0(j)))
                           ? 1
                           : 0);
      result = std::min(result, new_distance);
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

void saveToFile(const std::array<std::vector<Label>, 2>& labels,
                const std::vector<Vertex>& f, const std::string& fileName) {
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

  if (!f.empty()) {
    for (std::size_t i = 0; i < f.size(); ++i) {
      outFile << "f " << i << " " << f[i] << "\n";
    }
  }

  outFile.close();
}

template <typename TYPE_BITSET>
void saveToFile(
    const std::array<std::vector<BitParallelLabels<TYPE_BITSET>>, 2>& labels,
    const std::string& fileName) {
  StatusLog log("Save to file");
  std::ofstream outFile(fileName);

  if (!outFile.is_open()) {
    std::cerr << "Error: Unable to open file " << fileName << " for writing.\n";
    return;
  }

  std::size_t N = labels[FWD].size();

  outFile << "V " << N << "\n";
  outFile << "W " << (int)(sizeof(TYPE_BITSET) << 3) << "\n";

  for (std::size_t v = 0; v < N; ++v) {
    outFile << "o " << v;
    labels[FWD][v].doForAll([&outFile](const Vertex hub, const Distance dist,
                                       const TYPE_BITSET s_1,
                                       const TYPE_BITSET s_0) {
      outFile << " " << (int)hub << " " << (int)dist << " " << (int)s_1 << " "
              << (int)s_0;
    });
    outFile << "\n";

    outFile << "i " << v;
    labels[BWD][v].doForAll([&outFile](const Vertex hub, const Distance dist,
                                       const TYPE_BITSET s_1,
                                       const TYPE_BITSET s_0) {
      outFile << " " << (int)hub << " " << (int)dist << " " << (int)s_1 << " "
              << (int)s_0;
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

template <typename TYPE_BITSET>
std::size_t computeTotalBytes(
    const std::array<std::vector<BitParallelLabels<TYPE_BITSET>>, 2>& labels) {
  std::size_t totalBytes = 0;

  for (const auto& labelSet : labels) {
    for (const auto& label : labelSet) {
      totalBytes += sizeof(Label);
      totalBytes += label.hubs.capacity() * sizeof(Vertex);
      totalBytes += label.dists.capacity() * sizeof(Distance);
      totalBytes += label.bitsets_s[0].capacity() * sizeof(TYPE_BITSET);
      totalBytes += label.bitsets_s[1].capacity() * sizeof(TYPE_BITSET);
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