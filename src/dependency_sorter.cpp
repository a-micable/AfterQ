#include "cairn/dependency_sorter.h"
#include <functional>
#include <map>
#include <set>
#include <stdexcept>

namespace cairn {
std::vector<std::string> DependencySorter::sort(const std::vector<DependencyEdge>& edges) const {
  std::map<std::string, std::vector<std::string>> graph;
  std::set<std::string> nodes;
  for (const DependencyEdge& edge : edges) {
    const std::string before = edge.before.str();
    const std::string after = edge.after.str();
    graph[before].push_back(after);
    nodes.insert(before);
    nodes.insert(after);
  }
  std::map<std::string, int> state;
  std::vector<std::string> out;
  std::function<void(const std::string&)> visit = [&](const std::string& node) {
    if (state[node] == 1) {
      throw std::runtime_error("dependency cycle detected at " + node);
    }
    if (state[node] == 2) {
      return;
    }
    state[node] = 1;
    for (const std::string& next : graph[node]) {
      visit(next);
    }
    state[node] = 2;
    out.push_back(node);
  };
  for (const std::string& node : nodes) {
    visit(node);
  }
  std::reverse(out.begin(), out.end());
  return out;
}
}
