#pragma once

#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../external/status_log.h"
#include "graph.h"
#include "hub_labels.h"
#include "lookup_storage.h"
#include "types.h"
#include "utils.h"

struct PSL {
  const std::vector<std::size_t>& rank;
  std::array<const Graph*, 2> graphs;
  std::array<std::vector<Label>, 2> labels;
  std::size_t numThreads;

  PSL(const Graph* fwdGraph, const Graph* bwdGraph,
      const std::vector<std::size_t>& rank, std::size_t numThreads = 1)
      : rank(rank),
        graphs{fwdGraph, bwdGraph},
        labels{std::vector<Label>(fwdGraph->numVertices()),
               std::vector<Label>(fwdGraph->numVertices())},
        numThreads(numThreads) {}

  void showStats() const { showLabelStats(labels); }

  void printLabels() const {
    for (Vertex v = 0; v < graphs[FWD]->numVertices(); ++v) {
      std::cout << "Vertex " << v << "\nFWD\n";
      labels[FWD][v].print();
      std::cout << "BWD\n";
      labels[BWD][v].print();
    }
  }

  void run() {
    StatusLog log("Computing Hub-Labels");
    const std::size_t numVertices = graphs[FWD]->numVertices();
    const std::size_t chunkSize = (numVertices + numThreads - 1) / numThreads;

    std::vector<std::thread> workers;

    // lambda method to hide the parallel thread assignement over the vertices
    auto processVertices = [&](auto func) {
      workers.clear();
      for (std::size_t t = 0; t < numThreads; ++t) {
        workers.emplace_back([&, t]() {
          std::size_t start = t * chunkSize;
          std::size_t end = std::min(start + chunkSize, numVertices);
          func(t, static_cast<Vertex>(start), static_cast<Vertex>(end));
        });
      }
      for (auto& thread : workers) thread.join();
    };

    processVertices([&](std::size_t /* threadId */, Vertex start, Vertex end) {
      for (Vertex u = start; u < end; ++u) {
        labels[FWD][u].clear();
        labels[BWD][u].clear();
        labels[FWD][u].add(u, 0);
        labels[BWD][u].add(u, 0);
      }
    });

    // we add all edges, and at this point it could be that there are duplicate
    // entries.
    graphs[FWD]->doForAllEdges([&](Vertex from, Vertex to) {
      bool upwardEdge = (rank[from] < rank[to]);
      const DIRECTION dir = upwardEdge ? BWD : FWD;
      labels[dir][upwardEdge ? to : from].add(upwardEdge ? from : to, 1);
    });

    // the possible duplicate entries are remove here.
    processVertices([&](std::size_t /* threadId */, Vertex start, Vertex end) {
      for (Vertex u = start; u < end; ++u) {
        labels[FWD][u].sort();
        labels[FWD][u].removeDuplicateHubs();

        labels[BWD][u].sort();
        labels[BWD][u].removeDuplicateHubs();
      }
    });

    Distance d = 2;
    std::atomic<bool> exploreNewRound = true;

    std::vector<LookupStorage<Vertex>> candidates(
        numThreads, LookupStorage<Vertex>(numVertices));

    auto processDirection = [&](DIRECTION dir, const std::size_t threadId,
                                const Vertex start, const Vertex end) {
      for (Vertex u = start; u < end; ++u) {
        candidates[threadId].clear();
        graphs[dir]->relaxAllEdges(u, [&](Vertex /* from */, Vertex to) {
          labels[dir][to].doForAll([&](Vertex w, Distance dist) {
            if (dist == d - 1) candidates[threadId].add(w);
          });
        });

        Label lookup(labels[dir][u]);

        for (Vertex w : candidates[threadId].getStorage()) {
          if (rank[u] <= rank[w] || sub_query(labels[!dir][w], lookup, d) <= d)
            continue;
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

      processVertices([&](std::size_t threadId, Vertex start, Vertex end) {
        processDirection(FWD, threadId, start, end);
        processDirection(BWD, threadId, start, end);
      });
      d += 1;
    }

    // processVertices([&](std::size_t /* threadId */, std::size_t start,
    // std::size_t end) {
    //   for (Vertex u = static_cast<Vertex>(start); u <
    //   static_cast<Vertex>(end); ++u) {
    //         labels[FWD][u].sort();
    //         labels[BWD][u].sort();
    //       }
    // });
  }
};
