#pragma once

#include <bitset>
#include <cassert>
#include <compare>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include "../external/fast-cpp-csv-parser/csv.h"
#include "graph.h"
#include "types.h"
#include "utils.h"

/**
 * @brief A compact 64-bit event identifier that encodes route, trip, and stop
 * values.
 *
 * This implementation packs the values into a single 64-bit integer with the
 * following layout:
 * - Bits 63..36: routeID (28 bits)
 * - Bits 35..8 : tripPos (28 bits)
 * - Bits 7..0  : stopPos (8 bits)
 *
 * The getters and setters use bit shifting and masking to enforce a consistent
 * bit layout independent of the compiler’s bitfield ordering.
 */
struct Event {
  static constexpr std::uint32_t MAX_ROUTEID = (1U << 28) - 1;
  static constexpr std::uint32_t MAX_TRIPPOS = (1U << 28) - 1;
  static constexpr std::uint32_t MAX_STOPPOS = (1U << 8) - 1;

  // Bit offsets for each field.
  static constexpr int ROUTEID_SHIFT = 36;
  static constexpr int TRIPPOS_SHIFT = 8;
  static constexpr int STOPPOS_SHIFT = 0;

  // Masks for each field.
  static constexpr std::uint64_t ROUTEID_MASK =
      (static_cast<std::uint64_t>(MAX_ROUTEID)) << ROUTEID_SHIFT;
  static constexpr std::uint64_t TRIPPOS_MASK =
      (static_cast<std::uint64_t>(MAX_TRIPPOS)) << TRIPPOS_SHIFT;
  static constexpr std::uint64_t STOPPOS_MASK =
      static_cast<std::uint64_t>(MAX_STOPPOS);

  std::uint64_t data;

  // Default constructor.
  Event() : data(0) {}

  // Parameterized constructor with range assertions.
  Event(std::uint32_t r, std::uint32_t t, std::uint32_t s) : data(0) {
    setRouteID(r);
    setTripPos(t);
    setStopPos(s);
  }

  // Setters with assertions.
  void setRouteID(std::uint32_t r) {
    assert(r <= MAX_ROUTEID && "routeID out of range");
    data = (data & ~ROUTEID_MASK) |
           (static_cast<std::uint64_t>(r) << ROUTEID_SHIFT);
  }
  void setTripPos(std::uint32_t t) {
    assert(t <= MAX_TRIPPOS && "tripPos out of range");
    data = (data & ~TRIPPOS_MASK) |
           (static_cast<std::uint64_t>(t) << TRIPPOS_SHIFT);
  }
  void setStopPos(std::uint32_t s) {
    assert(s <= MAX_STOPPOS && "stopPos out of range");
    data = (data & ~STOPPOS_MASK) |
           (static_cast<std::uint64_t>(s) << STOPPOS_SHIFT);
  }

  // Getters.
  std::uint32_t getRouteID() const {
    return static_cast<std::uint32_t>((data & ROUTEID_MASK) >> ROUTEID_SHIFT);
  }
  std::uint32_t getTripPos() const {
    return static_cast<std::uint32_t>((data & TRIPPOS_MASK) >> TRIPPOS_SHIFT);
  }
  std::uint8_t getStopPos() const {
    return static_cast<std::uint8_t>((data & STOPPOS_MASK) >> STOPPOS_SHIFT);
  }

  // Compare the underlying 64-bit value.
  auto operator<=>(const Event& other) const { return data <=> other.data; }
  bool operator==(const Event& other) const = default;
};

std::ostream& operator<<(std::ostream& os, const Event& event) {
  // Casting getStopPos() to unsigned int to avoid printing a char.
  os << "Event(routeID: " << event.getRouteID()
     << ", tripPos: " << event.getTripPos()
     << ", stopPos: " << static_cast<unsigned>(event.getStopPos()) << ")";
  return os;
}

static_assert(sizeof(Event) == 8, "Event must fit in 64 bits");

struct TimeTable {
  Graph transferGraph;

  std::vector<std::size_t> rank;
  std::vector<Event> events;
  std::vector<std::uint8_t> stopsPerRoute;

  TimeTable() {};

  TimeTable(const std::string& fileName) {
    loadFromFile(fileName);
    computeRank();
  }

  std::size_t beginTripIndex(const Vertex v) const {
    assert(v < events.size());

    return v;
  }

  std::size_t endTripIndex(const Vertex v) const {
    assert(v < events.size());

    std::uint32_t routeID = events[v].getRouteID();
    std::uint8_t numStopsPerRoute = stopsPerRoute[routeID];

    assert(events[v].getStopPos() < numStopsPerRoute);

    return v + (numStopsPerRoute - events[v].getStopPos());
  }

  template <typename FUNC>
  void relaxTransfersOfRestOfTrip(const Vertex v, FUNC&& function) {
    assert(v < events.size());

    std::size_t start = beginTripIndex(v);
    std::size_t end = endTripIndex(v);

    for (std::size_t i = transferGraph.beginEdge(start);
         i < transferGraph.endEdge(end); ++i) {
      function(transferGraph.toVertex[i]);
    }
  }

  template <typename FUNC>
  void relaxEventsOfRestOfTrip(const Vertex v, FUNC&& function) {
    assert(v < events.size());

    std::size_t start = beginTripIndex(v);
    std::size_t end = endTripIndex(v);

    for (std::size_t i = start; i < end; ++i) {
      function(i);
    }
  }

  void loadFromFile(const std::string& fileName) {
    loadEvents(fileName + "/trips.csv");
    loadTransfrGraph(fileName + "/transfers.csv");
  }

  void loadTransfrGraph(const std::string& fileName) {
    std::vector<std::pair<Vertex, Vertex>> transfers;

    transfers.reserve(events.size());

    Vertex maxVertex = 0;

    io::CSVReader<2> in(fileName);
    in.read_header(io::ignore_extra_column, "FromVertex", "ToVertex");
    Vertex from, to;
    while (in.read_row(from, to)) {
      transfers.emplace_back(from, to);

      maxVertex = std::max(maxVertex, from);
      maxVertex = std::max(maxVertex, to);
    }

    transferGraph.clear();
    transferGraph.buildFromEdgeList(transfers);

    transferGraph.showStats();
  }

  void loadEvents(const std::string& fileName) {
    events.reserve(transferGraph.numVertices());

    io::CSVReader<4> in(fileName);
    in.read_header(io::ignore_extra_column, "StopEventId", "LineId", "TripId",
                   "StopIndex");
    Vertex from;
    std::uint32_t lineID, prevLineID(-1);
    std::uint32_t tripID, prevTripID(-1), tripPos(0);
    std::uint8_t stopPos, prevStopPos(-1);

    while (in.read_row(from, lineID, tripID, stopPos)) {
      if (lineID == prevLineID) {
        tripPos += (tripID != prevTripID);
      } else {
        tripPos = 0;
        if (prevStopPos != 255) stopsPerRoute.emplace_back(prevStopPos + 1);
      }

      prevLineID = lineID;
      prevTripID = tripID;
      prevStopPos = stopPos;

      events.emplace_back(lineID, tripPos, stopPos);
    }
  }

  void computeRank() {
    rank.resize(transferGraph.numVertices());
    std::iota(rank.begin(), rank.end(), 0);

    std::vector<std::size_t> randomNumber(transferGraph.numVertices());
    std::iota(randomNumber.begin(), randomNumber.end(), 0);

    std::mt19937 randomGenerator(42);
    std::shuffle(randomNumber.begin(), randomNumber.end(), randomGenerator);

    std::vector<std::size_t> degree(transferGraph.numVertices(), 0);
    transferGraph.doForAllEdges([&](const Vertex from, const Vertex to) {
      degree[from]++;
      degree[to]++;
    });

    std::sort(rank.begin(), rank.end(),
              [&](const std::size_t left, const std::size_t right) {
                return std::forward_as_tuple(degree[left], randomNumber[left]) >
                       std::forward_as_tuple(degree[right],
                                             randomNumber[right]);
              });
  }
};