#pragma once
#include "cairn/manifest.h"
#include <cstdint>
#include <map>
#include <vector>

namespace cairn {

struct ParseResult {
  std::shared_ptr<ManifestNode> root;
  std::map<std::string, std::shared_ptr<ManifestNode>> sections;
};

class Parser {
 public:
  explicit Parser(InternCache& cache);
  ParseResult parse(const std::vector<uint8_t>& bytes, bool verify_crc = true);

 private:
  ParseResult parse(const std::vector<uint8_t>& bytes, bool verify_crc, std::size_t depth);
  InternCache& cache_;
};
}
