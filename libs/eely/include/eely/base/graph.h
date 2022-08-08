#pragma once

#include "eely/base/assert.h"

#include <gsl/narrow>
#include <gsl/util>

#include <algorithm>
#include <functional>
#include <vector>

namespace eely {
// Simple directed graph.
template <typename TData>
class graph final {
public:
  // Vertex of a graph.
  struct vertex final {
    gsl::index id;
    TData data;
    std::vector<gsl::index> adjacency_list;

    bool operator==(const vertex& other) const;
  };

  // Get list of vertices.
  [[nodiscard]] const std::vector<vertex>& get_vertices() const;

  // Add vertex with specified data to the graph.
  // Returned vertex reference can be invalidated
  // if more vertices are pushed afterwards.
  const vertex& add_vertex(TData data);

  // Add directed edge between two specified vertices.
  void add_edge(gsl::index from, gsl::index to);

private:
  std::vector<vertex> _vertices;
};

// Traverse graph so that each vertex is visited only after all of its
// dependencies are visited.
// Graph traversal is written into `output_iterator`.
// If topological sort isn't possible (i.e. graph has cycle), return `false`.
template <typename TData, typename TOutputIterator>
[[nodiscard]] bool graph_topological_traversal(const graph<TData>& graph,
                                               TOutputIterator output_iterator);

// Implementation

template <typename TData>
bool graph<TData>::vertex::operator==(const vertex& other) const
{
  return id == other.id && data == other.data && adjacency_list == other.adjacency_list;
}

template <typename TData>
const std::vector<typename graph<TData>::vertex>& graph<TData>::get_vertices() const
{
  return _vertices;
}

template <typename TData>
const typename graph<TData>::vertex& graph<TData>::add_vertex(TData data)
{
  _vertices.push_back({.id = gsl::narrow<gsl::index>(_vertices.size()),
                       .data = std::move(data),
                       .adjacency_list = std::vector<gsl::index>{}});
  return _vertices.back();
}

template <typename TData>
void graph<TData>::add_edge(const gsl::index from, const gsl::index to)
{
  EXPECTS(from < _vertices.size() && to < _vertices.size());

  std::vector<gsl::index>& adjacency_list = _vertices[from].adjacency_list;
  EXPECTS(std::find(adjacency_list.begin(), adjacency_list.end(), to) == adjacency_list.end());

  adjacency_list.push_back(to);
}

template <typename TData, typename TOutputIterator>
bool graph_topological_traversal(const graph<TData>& graph, TOutputIterator output_iterator)
{
  using vertex = typename eely::graph<TData>::vertex;

  enum class mark {
    // Vertex is not visited
    none,

    // Vertex and its descendants are currently being visited
    temporary,

    // Vertex and all of its descendants have been visited
    permanent
  };

  const std::vector<vertex>& vertices = graph.get_vertices();

  std::vector<mark> marks{vertices.size(), mark::none};

  std::function<bool(const vertex&)> dfs_visitor;
  dfs_visitor = [&](const vertex& vertex) {
    if (marks[vertex.id] == mark::permanent) {
      return true;
    }

    if (marks[vertex.id] == mark::temporary) {
      return false;
    }

    marks[vertex.id] = mark::temporary;

    for (const gsl::index adjacent_vertex_id : vertex.adjacency_list) {
      if (!dfs_visitor(vertices[adjacent_vertex_id])) {
        return false;
      }
    }

    marks[vertex.id] = mark::permanent;

    *output_iterator = vertex.data;
    ++output_iterator;

    return true;
  };

  for (const vertex& vertex : vertices) {
    if (marks[vertex.id] == mark::none) {
      if (!dfs_visitor(vertex)) {
        // Cycle in a graph
        return false;
      }
    }
  }

  ENSURES(std::all_of(marks.begin(), marks.end(),
                      [](const mark mark) { return mark == mark::permanent; }));

  return true;
}
}  // namespace eely