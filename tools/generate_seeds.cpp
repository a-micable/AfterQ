#include "cairn/manifest_writer.h"
#include "cairn/intern_cache.h"
#include "cairn/scenario_corpus.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
void write_file(const std::string& dir, const std::string& name, const std::vector<uint8_t>& bytes) {
  std::ofstream out(dir + "/" + name, std::ios::binary);
  if (!out) {
    throw std::runtime_error("cannot write seed");
  }
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::vector<uint8_t> chain(int depth, int fields_per_section) {
  cairn::ManifestWriter w;
  for (int i = 0; i < depth; ++i) {
    const std::string section = i == 0 ? "root" : "s" + std::to_string(i);
    w.section(section);
    if (i + 1 < depth) {
      w.include("s" + std::to_string(i + 1));
    }
    for (int f = 0; f < fields_per_section; ++f) {
      w.field_string("k" + std::to_string(i) + "_" + std::to_string(f),
                     "v" + std::to_string(i) + "_" + std::to_string(f));
    }
  }
  return w.bytes();
}

std::vector<uint8_t> conditional_seed() {
  cairn::ManifestWriter nested;
  nested.section("root");
  nested.field_string("mode_detail", "fast-path");
  cairn::ManifestWriter w;
  w.section("root");
  w.field_string("mode", "fast");
  w.conditional("mode==fast", nested.bytes());
  return w.bytes();
}

std::vector<uint8_t> schema_seed() {
  cairn::InternCache cache(32);
  std::vector<cairn::SchemaField> fields;
  fields.push_back({cache.intern("name"), cairn::ValueType::String, true});
  fields.push_back({cache.intern("enabled"), cairn::ValueType::Boolean, true});
  cairn::ManifestWriter w;
  w.section("root");
  w.field_string("name", "demo");
  w.field_boolean("enabled", true);
  w.schema("root", fields);
  return w.bytes();
}
}

int main(int argc, char** argv) {
  try {
    const std::string dir = argc > 1 ? argv[1] : "fuzz/corpus/resolve_fuzzer";
    cairn::ManifestWriter a;
    a.section("root");
    a.field_string("name", "minimal");
    write_file(dir, "01_minimal_string.bin", a.bytes());
    cairn::ManifestWriter b;
    b.section("root");
    b.field_integer("answer", 42);
    b.field_boolean("enabled", true);
    write_file(dir, "02_minimal_typed.bin", b.bytes());
    write_file(dir, "03_chain_depth_2.bin", chain(2, 8));
    write_file(dir, "04_chain_depth_3.bin", chain(3, 8));
    write_file(dir, "05_chain_depth_4.bin", chain(4, 8));
    write_file(dir, "06_conditional.bin", conditional_seed());
    write_file(dir, "07_schema.bin", schema_seed());
    write_file(dir, "08_chain_depth_6_near_capacity.bin", chain(6, 10));
    for (std::size_t i = 0; i < cairn::scenario_count() && i < 24u; ++i) {
      write_file(dir, "conformance_" + std::to_string(i) + ".bin", cairn::build_scenario_manifest(i));
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
