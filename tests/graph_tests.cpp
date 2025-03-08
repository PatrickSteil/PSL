#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "../datastructures/graph.h"

void writeTestFile(const std::string &fileName, const std::string &content) {
  std::ofstream file(fileName);
  file << content;
  file.close();
}

class GraphTest : public ::testing::Test {
 protected:
  void SetUp() override {
    writeTestFile("test_edge_list.txt",
                  "1 2\n"
                  "1 3\n"
                  "2 3\n"
                  "3 4\n"
                  "4 1\n");

    writeTestFile("test_dimacs.txt",
                  "c This is a test DIMACS graph file\n"
                  "c with comments\n"
                  "p edge 4 5\n"
                  "a 1 2\n"
                  "a 1 3\n"
                  "a 2 3\n"
                  "a 3 4\n"
                  "a 4 1\n");
  }

  void TearDown() override {
    std::remove("test_edge_list.txt");
    std::remove("test_dimacs.txt");
  }
};

TEST_F(GraphTest, ReadFromEdgeList) {
  Graph graph;
  graph.readFromEdgeList("test_edge_list.txt");

  EXPECT_EQ(graph.numVertices(), 4);
  EXPECT_EQ(graph.numEdges(), 5);

  EXPECT_EQ(graph.beginEdge(0), 0);
  EXPECT_EQ(graph.endEdge(0), 2);

  EXPECT_EQ(graph.beginEdge(1), 2);
  EXPECT_EQ(graph.endEdge(1), 3);

  EXPECT_EQ(graph.beginEdge(2), 3);
  EXPECT_EQ(graph.endEdge(2), 4);

  EXPECT_EQ(graph.beginEdge(3), 4);
  EXPECT_EQ(graph.endEdge(3), 5);
}

TEST_F(GraphTest, ReadFromDimacs) {
  Graph graph;
  graph.readDimacs("test_dimacs.txt");

  EXPECT_EQ(graph.numVertices(), 4);
  EXPECT_EQ(graph.numEdges(), 5);

  EXPECT_EQ(graph.beginEdge(0), 0);
  EXPECT_EQ(graph.endEdge(0), 2);

  EXPECT_EQ(graph.beginEdge(1), 2);
  EXPECT_EQ(graph.endEdge(1), 3);

  EXPECT_EQ(graph.beginEdge(2), 3);
  EXPECT_EQ(graph.endEdge(2), 4);

  EXPECT_EQ(graph.beginEdge(3), 4);
  EXPECT_EQ(graph.endEdge(3), 5);
}

TEST_F(GraphTest, ReverseGraph) {
  Graph graph;
  graph.readFromEdgeList("test_edge_list.txt");
  Graph reversed = graph.reverseGraph();

  EXPECT_EQ(reversed.numVertices(), graph.numVertices());
  EXPECT_EQ(reversed.numEdges(), graph.numEdges());

  EXPECT_EQ(reversed.beginEdge(0), 0);
  EXPECT_EQ(reversed.endEdge(0), 1);

  EXPECT_EQ(reversed.beginEdge(1), 1);
  EXPECT_EQ(reversed.endEdge(1), 2);

  EXPECT_EQ(reversed.beginEdge(2), 2);
  EXPECT_EQ(reversed.endEdge(2), 4);

  EXPECT_EQ(reversed.beginEdge(3), 4);
  EXPECT_EQ(reversed.endEdge(3), 5);
}

TEST_F(GraphTest, ReorderGraph) {
  Graph graph;
  graph.readFromEdgeList("test_edge_list.txt");

  std::size_t oldNumVertices = graph.numVertices();
  std::size_t oldNumEdges = graph.numEdges();

  std::vector<std::size_t> rank(graph.numVertices(), 0);
  rank[0] = 2;
  rank[1] = 1;
  rank[2] = 0;
  rank[3] = 3;

  graph.reorderByRank(rank);

  EXPECT_EQ(graph.numVertices(), oldNumVertices);
  EXPECT_EQ(graph.numEdges(), oldNumEdges);

  EXPECT_EQ(graph.degree(0), 1);
  EXPECT_EQ(graph.degree(1), 1);
  EXPECT_EQ(graph.degree(2), 2);
  EXPECT_EQ(graph.degree(3), 1);

  EXPECT_EQ(graph.beginEdge(0), 0);
  EXPECT_EQ(graph.endEdge(0), 1);

  EXPECT_EQ(graph.beginEdge(1), 1);
  EXPECT_EQ(graph.endEdge(1), 2);

  EXPECT_EQ(graph.beginEdge(2), 2);
  EXPECT_EQ(graph.endEdge(2), 4);

  EXPECT_EQ(graph.beginEdge(3), 4);
  EXPECT_EQ(graph.endEdge(3), 5);
}