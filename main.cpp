#include <iostream>
#include <random>
#include <thread>

#include "datastructures/graph.h"
#include "datastructures/hub_labels.h"
#include "datastructures/psl.h"
#include "datastructures/psl_plus.h"
#include "external/cmdparser.hpp"

void configure_parser(cli::Parser &parser) {
  parser.set_required<std::string>("i", "input_graph",
                                   "Input graph file (in DIMACS format).");
  parser.set_optional<std::size_t>(
      "t", "number_threads",
      static_cast<std::size_t>(std::thread::hardware_concurrency()),
      "Number of threads to use.");
  parser.set_optional<std::string>("o", "output_file", "",
                                   "Output file to save hub labels into.");
  parser.set_optional<bool>(
      "s", "show_stats", false,
      "Show statistics about the graph, as well as the computed hub labels.");
  parser.set_optional<bool>(
      "p", "PSL+", false,
      "Removes equivalence vertices V1 and V2. This is the PSL+. If an output "
      "file is passed as argument, the mapping function f(v) will be exported "
      "as well.");
};

int main(int argc, char *argv[]) {
  cli::Parser parser(argc, argv,
                     "This code implements the PSL Hub Labeling algorithm for "
                     "directed graphs. It can be run in parallel, or "
                     "sequentially.\nPatrick Steil (2025)");
  configure_parser(parser);
  parser.run_and_exit_if_error();

  const std::string inputFileName = parser.get<std::string>("i");
  const std::size_t numberOfThreads = parser.get<std::size_t>("t");
  const std::string outputFileName = parser.get<std::string>("o");
  const bool printStats = parser.get<bool>("s");
  const bool pslPlus = parser.get<bool>("p");

  Graph g;
  // g.readFromEdgeList(inputFileName);
  g.readDimacs(inputFileName);

  if (printStats) g.showStats();

  // generate degree based rank
  std::vector<std::size_t> rank(g.numVertices());
  std::iota(rank.begin(), rank.end(), 0);

  std::vector<std::size_t> randomNumber(g.numVertices());
  std::iota(randomNumber.begin(), randomNumber.end(), 0);

  std::mt19937 randomGenerator(42);
  std::shuffle(randomNumber.begin(), randomNumber.end(), randomGenerator);

  std::vector<std::size_t> degree(g.numVertices(), 0);
  g.doForAllEdges([&](const Vertex from, const Vertex to) {
    degree[from]++;
    degree[to]++;
  });

  std::sort(rank.begin(), rank.end(),
            [&](const std::size_t left, const std::size_t right) {
              return std::forward_as_tuple(degree[left], randomNumber[left]) >
                     std::forward_as_tuple(degree[right], randomNumber[right]);
            });

  g.reorderByRank(rank);

  std::vector<Vertex> f;

  if (pslPlus) {
    auto [partition, mapping] = computePartitionAndF(g, numberOfThreads);

    g.removeVertices(partition, mapping);
    f = mapping;

    if (printStats) g.showStats();
  }

  Graph bwdGraph = g.reverseGraph();

  PSL psl(&g, &bwdGraph, numberOfThreads);
  psl.run();

  if (printStats) psl.showStats();

  if (!outputFileName.empty()) saveToFile(psl.labels, f, outputFileName);
}
