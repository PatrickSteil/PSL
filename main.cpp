#include <iostream>
#include <random>
#include <thread>

#include "datastructures/graph.h"
#include "datastructures/hub_labels.h"
#include "datastructures/psl.h"
#include "external/cmdparser.hpp"

void configure_parser(cli::Parser &parser) {
  parser.set_required<std::string>("i", "input_graph", "Input graph file.");
  parser.set_optional<std::size_t>(
      "t", "number_threads",
      static_cast<std::size_t>(std::thread::hardware_concurrency()),
      "Number of threads to use.");
  parser.set_optional<std::string>("o", "output_file", "",
                                   "Output file to save hub labels into.");
};

int main(int argc, char *argv[]) {
  cli::Parser parser(argc, argv);
  configure_parser(parser);
  parser.run_and_exit_if_error();

  const std::string inputFileName = parser.get<std::string>("i");
  const std::size_t numberOfThreads = parser.get<std::size_t>("t");
  const std::string outputFileName = parser.get<std::string>("o");

  Graph g;
  g.readDimacs(inputFileName);

  // g.removeEdges([&rank](const Vertex from, const Vertex to) {
  //   return rank[from] > rank[to];
  // });

  Graph bwdGraph = g.reverseGraph();

  g.showStats();
  bwdGraph.showStats();

  // generate degree based rank
  std::vector<std::size_t> rank(g.numVertices());
  std::iota(rank.begin(), rank.end(), 0);

  std::vector<std::size_t> randomNumber(g.numVertices());
  std::iota(randomNumber.begin(), randomNumber.end(), 0);

  std::mt19937 randomGenerator(42);
  std::shuffle(randomNumber.begin(), randomNumber.end(), randomGenerator);

  std::sort(rank.begin(), rank.end(),
            [&](const std::size_t left, const std::size_t right) {
              return std::forward_as_tuple(
                         g.degree(static_cast<Vertex>(left)) +
                             bwdGraph.degree(static_cast<Vertex>(left)),
                         randomNumber[left]) >
                     std::forward_as_tuple(
                         g.degree(static_cast<Vertex>(right)) +
                             bwdGraph.degree(static_cast<Vertex>(right)),
                         randomNumber[right]);
            });

  PSL psl(&g, &bwdGraph, rank, numberOfThreads);
  psl.run();
  psl.showStats();

  if (!outputFileName.empty()) saveToFile(psl.labels, outputFileName);
  // psl.printLabels();

  // benchmark_hublabels(psl.labels, 1000);
}
