#include "cairn/include_resolver.h"
#include <algorithm>
#include <stdexcept>

namespace cairn {

void IncludeRegistry::add_bytes(const std::string& name, const std::vector<uint8_t>& bytes) {
  bytes_[name] = bytes;
}

void IncludeRegistry::add_node(const std::string& name, std::shared_ptr<ManifestNode> node) {
  nodes_[name] = node;
}

std::shared_ptr<ManifestNode> IncludeRegistry::get(const std::string& name, Parser& parser) const {
  const auto node = nodes_.find(name);
  if (node != nodes_.end()) {
    return clone_node(*node->second);
  }
  const auto raw = bytes_.find(name);
  if (raw != bytes_.end()) {
    ParseResult parsed = parser.parse(raw->second);
    const auto found = parsed.sections.find(name);
    if (found != parsed.sections.end()) {
      return clone_node(*found->second);
    }
    return clone_node(*parsed.root);
  }
  throw std::runtime_error("include not found: " + name);
}

bool IncludeRegistry::has(const std::string& name) const {
  return nodes_.find(name) != nodes_.end() || bytes_.find(name) != bytes_.end();
}

std::vector<std::string> IncludeRegistry::names() const {
  std::vector<std::string> out;
  for (const auto& pair : nodes_) {
    out.push_back(pair.first);
  }
  for (const auto& pair : bytes_) {
    if (std::find(out.begin(), out.end(), pair.first) == out.end()) {
      out.push_back(pair.first);
    }
  }
  return out;
}

IncludeResolver::IncludeResolver(InternCache& cache, const IncludeRegistry& registry, std::size_t max_depth)
    : cache_(cache), registry_(registry), max_depth_(max_depth) {}

std::shared_ptr<ManifestNode> IncludeResolver::resolve(const ManifestNode& root) {
  std::set<std::string> path;
  return resolve_node(root, path, 0u);
}

std::shared_ptr<ManifestNode> IncludeResolver::resolve_node(const ManifestNode& node, std::set<std::string>& path, std::size_t depth) {
  if (depth > max_depth_) {
    throw std::runtime_error("include depth exceeded");
  }
  const std::string section_name = node.section.str();
  if (!path.insert(section_name).second) {
    throw std::runtime_error("include cycle detected at " + section_name);
  }
  auto out = clone_node(node);
  out->includes.clear();
  Parser parser(cache_);
  for (const InternedString& include : node.includes) {
    const std::string target = include.str();
    if (path.find(target) != path.end()) {
      throw std::runtime_error("include cycle detected at " + target);
    }
    std::shared_ptr<ManifestNode> included = registry_.get(target, parser);
    std::shared_ptr<ManifestNode> resolved = resolve_node(*included, path, depth + 1u);
    merge_into(*out, *resolved);
  }
  for (ConditionalBlock& block : out->conditionals) {
    if (block.body) {
      std::set<std::string> conditional_path;
      block.body = resolve_node(*block.body, conditional_path, depth + 1u);
    }
  }
  path.erase(section_name);
  return out;
}

void IncludeResolver::merge_into(ManifestNode& dst, const ManifestNode& src) {
  dst.fields.insert(dst.fields.end(), src.fields.begin(), src.fields.end());
  dst.schemas.insert(dst.schemas.end(), src.schemas.begin(), src.schemas.end());
  dst.dependencies.insert(dst.dependencies.end(), src.dependencies.begin(), src.dependencies.end());
  dst.conditionals.insert(dst.conditionals.end(), src.conditionals.begin(), src.conditionals.end());
}
}
