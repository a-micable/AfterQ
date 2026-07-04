#include "cairn/include_resolver.h"
#include "cairn/parser.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  try {
    std::vector<uint8_t> bytes(data, data + size);
    cairn::InternCache cache(32);
    cairn::Parser parser(cache);
    cairn::ParseResult parsed = parser.parse(bytes, true);
    cairn::IncludeRegistry registry;
    for (const auto& pair : parsed.sections) {
      registry.add_node(pair.first, pair.second);
    }
    cairn::IncludeResolver resolver(cache, registry, 16);
    std::shared_ptr<cairn::ManifestNode> resolved = resolver.resolve(*parsed.root);
    (void)resolved;
  } catch (...) {
  }
  return 0;
}
