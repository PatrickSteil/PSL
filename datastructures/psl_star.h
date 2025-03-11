#pragma once

#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../external/status_log.h"
#include "graph.h"
#include "hub_labels.h"
#include "lookup_storage.h"
#include "types.h"
#include "utils.h"

class PSLStar {
 public:
  PSLStar(const Graph* fwdGraph, const Graph* bwdGraph,
          std::size_t numThreads = 1)
      : localMaximum(fwdGraph->numVertices(), false),
        graphs{fwdGraph, bwdGraph},
        labels{std::vector<Label>(fwdGraph->numVertices()),
               std::vector<Label>(fwdGraph->numVertices())},
        numThreads(numThreads) {
    computeLocalMinima();
    buildNeighbours();
  }

  void run();
  void printNeighbours() const;
  void printLabels() const;
  void showStats() const { showLabelStats(labels); }

  std::vector<bool> localMaximum;
  std::array<const Graph*, 2> graphs;
  std::array<std::array<std::vector<std::vector<Vertex>>, 2>, 2> neighbours;
  std::array<std::vector<Label>, 2> labels;
  std::size_t numThreads;

  void computeLocalMinima();
  void buildNeighbours();
  std::vector<Vertex>& getN1(Vertex v, DIRECTION dir) {
    return neighbours[dir][0][v];
  }
  std::vector<Vertex>& getN2(Vertex v, DIRECTION dir) {
    return neighbours[dir][1][v];
  }
};

void PSLStar::computeLocalMinima() {
  std::size_t numVertices = graphs[FWD]->numVertices();
  localMaximum.assign(numVertices, false);
  std::size_t counter = 0;

  for (Vertex v = 0; v < numVertices; ++v) {
    bool isMin = true;

    graphs[FWD]->relaxAllEdges(
        v, [&](Vertex from, Vertex to) { isMin &= (from > to); });
    graphs[BWD]->relaxAllEdges(
        v, [&](Vertex from, Vertex to) { isMin &= (from > to); });

    localMaximum[v] = isMin;
    counter += isMin;
  }
  std::cout << "[INFO] " << counter << " local minima found." << std::endl;
}

void PSLStar::buildNeighbours() {
  std::size_t numVertices = graphs[FWD]->numVertices();
  for (auto& dir : neighbours) {
    for (auto& order : dir) {
      order.resize(numVertices);
    }
  }

  auto computeNeighbours = [&](DIRECTION dir) {
    for (Vertex v = 0; v < numVertices; ++v) {
      if (localMaximum[v]) continue;

      std::unordered_set<Vertex> n2Candidates;
      graphs[dir]->relaxAllEdges(v, [&](Vertex, Vertex to) {
        if (localMaximum[to]) {
          n2Candidates.insert(to);
        } else {
          neighbours[dir][0][v].push_back(to);
        }
      });

      for (Vertex hub : n2Candidates) {
        graphs[dir]->relaxAllEdges(hub, [&](Vertex, Vertex to) {
          if (to != v) {
            neighbours[dir][1][v].push_back(to);
          }
        });
      }
    }
  };

  computeNeighbours(FWD);
  computeNeighbours(BWD);
}

void PSLStar::printNeighbours() const {
  for (Vertex v = 0; v < neighbours[FWD][0].size(); ++v) {
    std::cout << "Vertex " << v << ":\n";
    for (DIRECTION dir : {FWD, BWD}) {
      std::cout << "  " << (dir == FWD ? "FWD" : "BWD") << " Neighbors:\n";
      for (const auto& n : neighbours[dir][0][v]) {
        std::cout << "    " << n << "\n";
      }
      std::cout << "  " << (dir == FWD ? "FWD" : "BWD")
                << " Second-order Neighbors:\n";
      for (const auto& n : neighbours[dir][1][v]) {
        std::cout << "    " << n << "\n";
      }
    }
  }
}

void PSLStar::printLabels() const {
  for (Vertex v = 0; v < graphs[FWD]->numVertices(); ++v) {
    std::cout << "Vertex " << v << "\nFWD\n";
    labels[FWD][v].print();
    std::cout << "BWD\n";
    labels[BWD][v].print();
  }
}

void PSLStar::run() {
  StatusLog log("Computing Hub-Labels");
  const std::size_t numVertices = graphs[FWD]->numVertices();

  std::vector<Vertex> roots;
  roots.reserve(numVertices);

  for (Vertex v = 0; v < numVertices; ++v) {
    if (!localMaximum[v]) roots.emplace_back(v);
  }

  // const std::size_t chunkSizeVertices = (numVertices + numThreads - 1) /
  // numThreads;
  const std::size_t chunkSizeRoots =
      (roots.size() + numThreads - 1) / numThreads;

  std::vector<std::thread> workers;

  // lambda method to hide the parallel thread assignement over the vertices
  auto processVertices = [&](auto func) {
    workers.clear();
    for (std::size_t t = 0; t < numThreads; ++t) {
      workers.emplace_back([&, t]() {
        std::size_t start = t * chunkSizeRoots;
        std::size_t end = std::min(start + chunkSizeRoots, roots.size());
        func(t, start, end);
      });
    }
    for (auto& thread : workers) thread.join();
  };

  processVertices(
      [&](std::size_t /* threadId */, std::size_t start, std::size_t end) {
        for (std::size_t i = start; i < end; ++i) {
          Vertex u = roots[i];
          labels[FWD][u].clear();
          labels[BWD][u].clear();
          labels[FWD][u].add(u, 0);
          labels[BWD][u].add(u, 0);
        }
      });

  // This can add duplicate entries, which will be removed afterwards.
  processVertices(
      [&](std::size_t /* threadId */, std::size_t start, std::size_t end) {
        for (std::size_t i = start; i < end; ++i) {
          Vertex u = roots[i];
          for (const Vertex to : getN1(u, FWD)) {
            bool upwardEdge = (u < to);
            const DIRECTION dir = upwardEdge ? BWD : FWD;
            labels[dir][upwardEdge ? to : u].add(upwardEdge ? u : to, 1);
          }
        }
      });

  // the possible duplicate entries are remove here.
  processVertices(
      [&](std::size_t /* threadId */, std::size_t start, std::size_t end) {
        for (std::size_t i = start; i < end; ++i) {
          Vertex u = roots[i];

          labels[FWD][u].sort();
          labels[FWD][u].removeDuplicateHubs();

          labels[BWD][u].sort();
          labels[BWD][u].removeDuplicateHubs();

          assert(std::is_sorted(labels[FWD][u].hubs.begin(),
                                labels[FWD][u].hubs.end()));
          assert(std::is_sorted(labels[BWD][u].hubs.begin(),
                                labels[BWD][u].hubs.end()));
        }
      });

  Distance d = 2;
  std::atomic<bool> exploreNewRound = true;

  std::vector<LookupStorage<Vertex>> candidates(
      numThreads, LookupStorage<Vertex>(numVertices));

  auto processDirection = [&](DIRECTION dir, const std::size_t threadId,
                              std::size_t start, std::size_t end) {
    for (std::size_t i = start; i < end; ++i) {
      Vertex u = roots[i];

      candidates[threadId].clear();

      for (Vertex to : getN1(u, dir)) {
        labels[dir][to].doForAll([&](Vertex w, Distance dist) {
          if (dist == d - 1) candidates[threadId].add(w);
        });
      }

      for (Vertex to : getN2(u, dir)) {
        labels[dir][to].doForAll([&](Vertex w, Distance dist) {
          if (dist == d - 2) candidates[threadId].add(w);
        });
      }

      Label lookup(labels[dir][u]);

      for (Vertex w : candidates[threadId].getStorage()) {
        if (u <= w || sub_query(labels[!dir][w], lookup, d) <= d) continue;
        labels[dir][u].add(w, d);
        exploreNewRound.store(true, std::memory_order_relaxed);
      }

      // we keep all labels sorted as to allow for faster
      labels[dir][u].sort();
    }
  };

  // the main algorithm. each iteration of this while loop finds new hubs with
  // one more distance than in the previous round
  while (exploreNewRound) {
    exploreNewRound.store(false, std::memory_order_relaxed);

    processVertices(
        [&](std::size_t threadId, std::size_t start, std::size_t end) {
          processDirection(FWD, threadId, start, end);
          processDirection(BWD, threadId, start, end);
        });
    d += 1;
  }
}