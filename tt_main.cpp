#include <iostream>

#include "datastructures/tt.h"
#include "external/cmdparser.hpp"

void configure_parser(cli::Parser &parser) {
  parser.set_required<std::string>("i", "timetable_directory",
                                   "TimeTable directory (in CSV format).");
};

int main(int argc, char *argv[]) {
  cli::Parser parser(argc, argv, "Patrick Steil (2025)");
  configure_parser(parser);
  parser.run_and_exit_if_error();

  const std::string inputFileName = parser.get<std::string>("i");
  TimeTable tt(inputFileName);

  std::size_t maxRank(-1);
  std::size_t maxVertex(-1);

  tt.relaxEventsOfRestOfTrip(200, [&](const Vertex to) {
    if (tt.rank[to] < maxRank) {
      maxRank = tt.rank[to];
      maxVertex = to;
    }

    std::cout << tt.events[to] << " " << tt.rank[to] << " " << maxRank << " "
              << maxVertex << std::endl;
  });

  std::cout << "Relax" << std::endl;

  maxRank = -1;
  maxVertex = -1;

  tt.relaxTransfersOfRestOfTrip(200, [&](const Vertex to) {
    if (tt.rank[to] < maxRank) {
      maxRank = tt.rank[to];
      maxVertex = to;
    }

    std::cout << tt.events[to] << " " << tt.rank[to] << " " << maxRank << " "
              << maxVertex << std::endl;
  });
}