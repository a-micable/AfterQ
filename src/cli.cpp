#include "cairn/evaluator.h"
#include "cairn/include_resolver.h"
#include "cairn/parser.h"
#include <fstream>
#include <iostream>
#include <iterator>

namespace {
std::vector<uint8_t> read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("cannot open " + path);
  }
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void load_registry(const cairn::ParseResult& parsed, cairn::IncludeRegistry& registry) {
  for (const auto& pair : parsed.sections) {
    registry.add_node(pair.first, pair.second);
  }
}
}

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      std::cerr << "usage: cairn <parse|eval|validate> <manifest> [registry]\n";
      return 2;
    }
    cairn::InternCache cache(256);
    cairn::Parser parser(cache);
    const std::vector<uint8_t> bytes = read_file(argv[2]);
    cairn::ParseResult parsed = parser.parse(bytes, true);
    const std::string command = argv[1];
    if (command == "parse") {
      cairn::print_node(std::cout, *parsed.root);
      return 0;
    }
    cairn::IncludeRegistry registry;
    load_registry(parsed, registry);
    if (argc > 3) {
      cairn::ParseResult registry_parsed = parser.parse(read_file(argv[3]), true);
      load_registry(registry_parsed, registry);
    }
    cairn::IncludeResolver resolver(cache, registry, 64);
    std::shared_ptr<cairn::ManifestNode> resolved = resolver.resolve(*parsed.root);
    cairn::Evaluator evaluator;
    if (command == "eval") {
      cairn::EvaluationResult result = evaluator.evaluate(*resolved);
      for (const std::string& error : result.errors) {
        std::cerr << error << '\n';
      }
      if (!result.ok()) {
        return 1;
      }
      for (const auto& pair : result.values) {
        std::cout << pair.first << '=' << cairn::value_to_string(pair.second) << '\n';
      }
      return 0;
    }
    if (command == "validate") {
      std::vector<std::string> errors = evaluator.validate(*resolved);
      for (const std::string& error : errors) {
        std::cerr << error << '\n';
      }
      return errors.empty() ? 0 : 1;
    }
    std::cerr << "unknown command\n";
    return 2;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
