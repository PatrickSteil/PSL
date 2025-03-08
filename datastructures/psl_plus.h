#pragma once

#include <thread>
#include <unordered_map>
#include <vector>

#include "../external/status_log.h"
#include "graph.h"
#include "types.h"

// This method compute the partitions V1, V2 and V3 for each vertex, as well as
// the representation for each vertex f(v)
std::pair<std::vector<std::uint8_t>, std::vector<Vertex>> computePartitionAndF(
    const Graph &G, const std::size_t numThreads = 1) {
  StatusLog log("Reducing the graph");
  const std::size_t n = G.numVertices();
  const std::size_t chunkSizeVertices = (n + numThreads - 1) / numThreads;

  std::vector<std::thread> workers;

  auto processVertices = [&](auto func) {
    workers.clear();
    for (std::size_t t = 0; t < numThreads; ++t) {
      workers.emplace_back([&, t]() {
        std::size_t start = t * chunkSizeVertices;
        std::size_t end = std::min(start + chunkSizeVertices, n);
        func(t, static_cast<Vertex>(start), static_cast<Vertex>(end));
      });
    }
    for (auto &thread : workers) thread.join();
  };

  std::vector<std::vector<Vertex>> openAdj(n), closedAdj(n);

  processVertices([&](const std::size_t /* threadId */, const Vertex left,
                      const Vertex right) {
    for (Vertex u = left; u < right; ++u) {
      auto start = G.beginEdge(u);
      auto end = G.endEdge(u);
      openAdj[u].reserve(end - start);
      for (std::size_t i = start; i < end; i++) {
        openAdj[u].push_back(G.toVertex[i]);
      }
      std::sort(openAdj[u].begin(), openAdj[u].end());
    }
  });

  processVertices([&](const std::size_t /* threadId */, const Vertex left,
                      const Vertex right) {
    for (Vertex u = left; u < right; ++u) {
      closedAdj[u] = openAdj[u];
      closedAdj[u].push_back(u);
      std::sort(closedAdj[u].begin(), closedAdj[u].end());
      closedAdj[u].erase(std::unique(closedAdj[u].begin(), closedAdj[u].end()),
                         closedAdj[u].end());
    }
  });

  struct RepCount {
    Vertex rep;
    size_t count;
  };

  struct VecHash {
    std::size_t operator()(const std::vector<Vertex> &vec) const {
      std::size_t h = 0;
      for (auto &x : vec) {
        h = 31ULL * h + std::hash<Vertex>()(x);
      }
      return h;
    }
  };
  struct VecEqual {
    bool operator()(const std::vector<Vertex> &a,
                    const std::vector<Vertex> &b) const {
      return a == b;
    }
  };

  std::unordered_map<std::vector<Vertex>, RepCount, VecHash, VecEqual> openMap;
  std::unordered_map<std::vector<Vertex>, RepCount, VecHash, VecEqual>
      closedMap;
  openMap.reserve(n);
  closedMap.reserve(n);

  for (Vertex u = 0; u < n; ++u) {
    auto &key = openAdj[u];
    auto it = openMap.find(key);
    if (it == openMap.end()) {
      openMap[key] = {u, 1};
    } else {
      if (u < it->second.rep) {
        it->second.rep = u;
      }
      it->second.count += 1;
    }
  }

  for (Vertex u = 0; u < n; ++u) {
    auto &key = closedAdj[u];
    auto it = closedMap.find(key);
    if (it == closedMap.end()) {
      closedMap[key] = {u, 1};
    } else {
      if (u < it->second.rep) {
        it->second.rep = u;
      }
      it->second.count += 1;
    }
  }

  std::vector<std::uint8_t> partition(n, 0);
  std::vector<Vertex> f(n, 0);

  processVertices([&](const std::size_t /* threadId */, const Vertex left,
                      const Vertex right) {
    for (Vertex u = left; u < right; ++u) {
      const auto &oKey = openAdj[u];
      const auto &oVal = openMap[oKey];
      if (oVal.count >= 2) {
        partition[u] = 1;
        f[u] = oVal.rep;
      } else {
        const auto &cKey = closedAdj[u];
        const auto &cVal = closedMap[cKey];
        if (cVal.count >= 2) {
          partition[u] = 2;
          f[u] = cVal.rep;
        } else {
          partition[u] = 3;
          f[u] = u;
        }
      }
    }
  });

  return {partition, f};
}