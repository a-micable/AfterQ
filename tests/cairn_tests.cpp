#include "cairn/dependency_sorter.h"
#include "cairn/evaluator.h"
#include "cairn/include_resolver.h"
#include "cairn/manifest_writer.h"
#include "cairn/parser.h"
#include "cairn/scenario_corpus.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::vector<uint8_t> chain(int depth, int fields) {
  cairn::ManifestWriter w;
  for (int i = 0; i < depth; ++i) {
    const std::string section = i == 0 ? "root" : "s" + std::to_string(i);
    w.section(section);
    if (i + 1 < depth) {
      w.include("s" + std::to_string(i + 1));
    }
    for (int f = 0; f < fields; ++f) {
      w.field_string("k" + std::to_string(i) + "_" + std::to_string(f), "v" + std::to_string(i) + "_" + std::to_string(f));
    }
  }
  return w.bytes();
}

cairn::ParseResult parse(cairn::InternCache& cache, const std::vector<uint8_t>& bytes) {
  cairn::Parser parser(cache);
  return parser.parse(bytes);
}

std::shared_ptr<cairn::ManifestNode> resolve_all(cairn::InternCache& cache, const cairn::ParseResult& parsed) {
  cairn::IncludeRegistry registry;
  for (const auto& pair : parsed.sections) {
    registry.add_node(pair.first, pair.second);
  }
  cairn::IncludeResolver resolver(cache, registry, 16);
  return resolver.resolve(*parsed.root);
}

void write_temp(const std::string& path, const std::vector<uint8_t>& bytes) {
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void expect(bool value, const std::string& name) {
  if (!value) {
    std::cerr << "failed: " << name << '\n';
    std::abort();
  }
}
}

int main() {
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.field_string("name", "cairn");
    cairn::InternCache cache(16);
    auto parsed = parse(cache, w.bytes());
    expect(parsed.root->fields.size() == 1u, "string field round trip");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.field_integer("n", 7);
    w.field_boolean("b", true);
    w.field_float_array("xs", {1.0, 2.5});
    cairn::InternCache cache(16);
    auto parsed = parse(cache, w.bytes());
    expect(parsed.root->fields.size() == 3u, "typed field round trip");
  }
  {
    cairn::InternCache cache(16);
    auto parsed = parse(cache, chain(2, 1));
    auto resolved = resolve_all(cache, parsed);
    expect(resolved->fields.size() == 2u, "include depth 1");
  }
  {
    cairn::InternCache cache(32);
    auto parsed = parse(cache, chain(3, 1));
    auto resolved = resolve_all(cache, parsed);
    expect(resolved->fields.size() == 3u, "include depth 2");
  }
  {
    cairn::InternCache cache(32);
    auto parsed = parse(cache, chain(4, 1));
    auto resolved = resolve_all(cache, parsed);
    expect(resolved->fields.size() == 4u, "include depth 3");
  }
  {
    cairn::InternCache cache(64);
    auto parsed = parse(cache, chain(5, 1));
    auto resolved = resolve_all(cache, parsed);
    expect(resolved->fields.size() == 5u, "include depth 4");
  }
  {
    cairn::InternCache cache(2);
    cairn::InternedString a = cache.intern("a");
    cairn::InternedString b = cache.intern("b");
    cairn::InternedString c = cache.intern("c");
    expect(a.str() == "a" && b.str() == "b" && c.str() == "c", "eviction keeps handles alive");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.include("a");
    w.section("a");
    w.include("root");
    cairn::InternCache cache(16);
    auto parsed = parse(cache, w.bytes());
    bool threw = false;
    try {
      (void)resolve_all(cache, parsed);
    } catch (...) {
      threw = true;
    }
    expect(threw, "include cycle");
  }
  {
    cairn::InternCache cache(16);
    std::vector<cairn::DependencyEdge> edges;
    edges.push_back({cache.intern("a"), cache.intern("b")});
    edges.push_back({cache.intern("b"), cache.intern("c")});
    cairn::DependencySorter sorter;
    auto order = sorter.sort(edges);
    expect(order.size() == 3u && order.front() == "a", "dependency sort");
  }
  {
    cairn::InternCache cache(16);
    std::vector<cairn::DependencyEdge> edges;
    edges.push_back({cache.intern("a"), cache.intern("b")});
    edges.push_back({cache.intern("b"), cache.intern("a")});
    cairn::DependencySorter sorter;
    bool threw = false;
    try {
      (void)sorter.sort(edges);
    } catch (...) {
      threw = true;
    }
    expect(threw, "dependency cycle");
  }
  {
    cairn::InternCache cache(32);
    std::vector<cairn::SchemaField> fields;
    fields.push_back({cache.intern("name"), cairn::ValueType::String, true});
    cairn::ManifestWriter w;
    w.section("root");
    w.field_string("name", "ok");
    w.schema("root", fields);
    auto parsed = parse(cache, w.bytes());
    cairn::Evaluator e;
    expect(e.validate(*parsed.root).empty(), "schema accept");
  }
  {
    cairn::InternCache cache(32);
    std::vector<cairn::SchemaField> fields;
    fields.push_back({cache.intern("name"), cairn::ValueType::String, true});
    cairn::ManifestWriter w;
    w.section("root");
    w.schema("root", fields);
    auto parsed = parse(cache, w.bytes());
    cairn::Evaluator e;
    expect(!e.validate(*parsed.root).empty(), "schema reject");
  }
  {
    cairn::ManifestWriter nested;
    nested.section("root");
    nested.field_string("chosen", "yes");
    cairn::ManifestWriter w;
    w.section("root");
    w.field_string("mode", "fast");
    w.conditional("mode==fast", nested.bytes());
    cairn::InternCache cache(32);
    auto parsed = parse(cache, w.bytes());
    cairn::Evaluator e;
    auto result = e.evaluate(*parsed.root);
    expect(result.values.count("chosen") == 1u, "conditional eval");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.field_string("name", "cairn");
    w.field_string("msg", "hello ${name}");
    cairn::InternCache cache(32);
    auto parsed = parse(cache, w.bytes());
    cairn::Evaluator e;
    auto result = e.evaluate(*parsed.root);
    expect(cairn::value_to_string(result.values["msg"]) == "hello cairn", "variable substitution");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.field_string("x", "y");
    auto bytes = w.bytes();
    bytes.back() ^= 0xffu;
    cairn::InternCache cache(32);
    bool threw = false;
    try {
      (void)parse(cache, bytes);
    } catch (...) {
      threw = true;
    }
    expect(threw, "crc reject");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.field_string("x", "y");
    write_temp("/tmp/cairn_cli.bin", w.bytes());
    expect(std::system("./cairn validate /tmp/cairn_cli.bin >/tmp/cairn_validate.out") == 0, "cli validate");
    expect(std::system("./cairn eval /tmp/cairn_cli.bin >/tmp/cairn_eval.out") == 0, "cli eval");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.include("missing");
    cairn::InternCache cache(8);
    auto parsed = parse(cache, w.bytes());
    bool threw = false;
    try {
      (void)resolve_all(cache, parsed);
    } catch (...) {
      threw = true;
    }
    expect(threw, "missing include");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.dependency("a", "b");
    cairn::InternCache cache(16);
    auto parsed = parse(cache, w.bytes());
    expect(parsed.root->dependencies.size() == 1u, "dependency record parse");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.include("child");
    cairn::InternCache cache(16);
    auto parsed = parse(cache, w.bytes());
    expect(parsed.root->includes.size() == 1u, "include record parse");
  }
  {
    cairn::ManifestWriter nested;
    nested.section("root");
    nested.field_integer("nested", 1);
    cairn::ManifestWriter w;
    w.section("root");
    w.conditional("flag", nested.bytes());
    cairn::InternCache cache(32);
    auto parsed = parse(cache, w.bytes());
    expect(parsed.root->conditionals.size() == 1u, "conditional record parse");
  }
  {
    cairn::InternCache cache(4);
    auto parsed = parse(cache, chain(6, 4));
    auto resolved = resolve_all(cache, parsed);
    expect(resolved->fields.size() == 24u, "deep cache eviction during resolve safe");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.field_string("z", "last");
    cairn::InternCache cache(1);
    auto parsed = parse(cache, w.bytes());
    expect(parsed.root->fields[0].key.str() == "z", "tiny cache key survives");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.field_boolean("flag", true);
    cairn::InternCache cache(16);
    auto parsed = parse(cache, w.bytes());
    cairn::Evaluator e;
    auto result = e.evaluate(*parsed.root);
    expect(cairn::value_to_string(result.values["flag"]) == "true", "boolean eval");
  }
  {
    cairn::ManifestWriter w;
    w.section("root");
    w.field_float_array("xs", {1.25, 2.5});
    cairn::InternCache cache(16);
    auto parsed = parse(cache, w.bytes());
    expect(cairn::value_to_string(parsed.root->fields[0].value).find("1.25") != std::string::npos, "float array text");
  }
  {
    cairn::ManifestWriter w;
    w.section("custom");
    w.field_string("x", "y");
    cairn::InternCache cache(16);
    auto parsed = parse(cache, w.bytes());
    expect(parsed.sections.count("custom") == 1u, "section record");
  }
  {
    expect(cairn::scenario_count() >= 1000u, "scenario corpus size");
    cairn::InternCache cache(128);
    for (std::size_t i = 0; i < 12u; ++i) {
      auto parsed = parse(cache, cairn::build_scenario_manifest(i));
      auto resolved = resolve_all(cache, parsed);
      expect(!resolved->fields.empty(), "scenario manifest resolves");
    }
  }
  std::cout << "25 tests passed\n";
}
