#pragma once
#include "cairn/parser.h"
#include <map>
#include <set>

namespace cairn {

class IncludeRegistry {
 public:
  void add_bytes(const std::string& name, const std::vector<uint8_t>& bytes);
  void add_node(const std::string& name, std::shared_ptr<ManifestNode> node);
  std::shared_ptr<ManifestNode> get(const std::string& name, Parser& parser) const;
  bool has(const std::string& name) const;
  std::vector<std::string> names() const;

 private:
  std::map<std::string, std::vector<uint8_t>> bytes_;
  std::map<std::string, std::shared_ptr<ManifestNode>> nodes_;
};

class IncludeResolver {
 public:
  IncludeResolver(InternCache& cache, const IncludeRegistry& registry, std::size_t max_depth = 32);
  std::shared_ptr<ManifestNode> resolve(const ManifestNode& root);

 private:
  InternCache& cache_;
  const IncludeRegistry& registry_;
  std::size_t max_depth_;
  std::shared_ptr<ManifestNode> resolve_node(const ManifestNode& node, std::set<std::string>& path, std::size_t depth);
  void merge_into(ManifestNode& dst, const ManifestNode& src);
};
}
