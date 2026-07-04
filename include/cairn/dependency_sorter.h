#pragma once
#include "cairn/manifest.h"
#include <string>
#include <vector>

namespace cairn {
class DependencySorter {
 public:
  std::vector<std::string> sort(const std::vector<DependencyEdge>& edges) const;
};
}
