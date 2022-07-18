#include <eely/graph.h>

#include <gtest/gtest.h>

#include <cstdarg>
#include <iterator>

TEST(graph, graph)
{
  using namespace eely;

  graph<int> graph;

  graph.add_vertex(0);
  graph.add_vertex(1);
  graph.add_vertex(2);

  const auto& vertices{graph.get_vertices()};
  const auto& v0{vertices[0]};
  const auto& v1{vertices[1]};
  const auto& v2{vertices[2]};

  EXPECT_EQ(v0.data, 0);
  EXPECT_EQ(v1.data, 1);
  EXPECT_EQ(v2.data, 2);

  graph.add_edge(v0.id, v1.id);
  graph.add_edge(v1.id, v2.id);
  EXPECT_EQ(v0.adjacency_list, (std::vector{v1.id}));
  EXPECT_EQ(v1.adjacency_list, (std::vector{v2.id}));

  std::vector<int> traverse_results;
  bool traversed{graph_topological_traversal(graph, std::back_inserter(traverse_results))};
  EXPECT_TRUE(traversed);
  EXPECT_EQ(traverse_results.size(), 3);
  EXPECT_EQ(traverse_results, (std::vector<int>{2, 1, 0}));

  // Introduce a cycle
  graph.add_edge(v2.id, v0.id);
  traversed = graph_topological_traversal(graph, std::back_inserter(traverse_results));
  EXPECT_FALSE(traversed);
}