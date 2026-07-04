#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
  const std::string path = argc > 1 ? argv[1] : "src/scenario_corpus.cpp";
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("cannot open output");
  }
  out << "#include \"cairn/scenario_corpus.h\"\n";
  out << "#include \"cairn/manifest_writer.h\"\n";
  out << "#include \"cairn/intern_cache.h\"\n";
  out << "#include <stdexcept>\n";
  out << "\n";
  out << "namespace cairn {\n";
  out << "namespace {\n";
  out << "const ScenarioSpec kScenarioSpecs[] = {\n";
  for (int i = 0; i < 1220; ++i) {
    const int depth = 1 + (i % 8);
    const int fields = 1 + ((i / 8) % 12);
    const bool schema = (i % 3) == 0;
    const bool conditionals = (i % 4) == 0;
    const bool dependencies = depth > 1 && (i % 5) == 0;
    const int cache = 8 + (i % 64);
    out << "  {\n";
    out << "    \"scenario_" << std::setw(4) << std::setfill('0') << i << "\",\n";
    out << "    " << depth << ",\n";
    out << "    " << fields << ",\n";
    out << "    " << (schema ? "true" : "false") << ",\n";
    out << "    " << (conditionals ? "true" : "false") << ",\n";
    out << "    " << (dependencies ? "true" : "false") << ",\n";
    out << "    " << cache << "\n";
    out << "  },\n";
  }
  out << "};\n";
  out << "}\n";
  out << "\n";
  out << "std::size_t scenario_count() {\n";
  out << "  return sizeof(kScenarioSpecs) / sizeof(kScenarioSpecs[0]);\n";
  out << "}\n";
  out << "\n";
  out << "const ScenarioSpec& scenario_at(std::size_t index) {\n";
  out << "  if (index >= scenario_count()) {\n";
  out << "    throw std::out_of_range(\"scenario index out of range\");\n";
  out << "  }\n";
  out << "  return kScenarioSpecs[index];\n";
  out << "}\n";
  out << "\n";
  out << "std::vector<uint8_t> build_scenario_manifest(const ScenarioSpec& spec) {\n";
  out << "  ManifestWriter writer;\n";
  out << "  for (int section_index = 0; section_index < spec.depth; ++section_index) {\n";
  out << "    const std::string section = section_index == 0 ? std::string(\"root\") : std::string(\"section_\") + std::to_string(section_index);\n";
  out << "    writer.section(section);\n";
  out << "    if (section_index + 1 < spec.depth) {\n";
  out << "      writer.include(std::string(\"section_\") + std::to_string(section_index + 1));\n";
  out << "    }\n";
  out << "    for (int field_index = 0; field_index < spec.fields_per_section; ++field_index) {\n";
  out << "      const std::string key = std::string(\"field_\") + std::to_string(section_index) + \"_\" + std::to_string(field_index);\n";
  out << "      const int selector = (section_index + field_index + spec.cache_hint) % 4;\n";
  out << "      if (selector == 0) {\n";
  out << "        writer.field_string(key, std::string(spec.name) + \"_value_\" + std::to_string(section_index) + \"_\" + std::to_string(field_index));\n";
  out << "      } else if (selector == 1) {\n";
  out << "        writer.field_integer(key, static_cast<int64_t>(section_index * 1000 + field_index));\n";
  out << "      } else if (selector == 2) {\n";
  out << "        writer.field_boolean(key, (field_index % 2) == 0);\n";
  out << "      } else {\n";
  out << "        writer.field_float_array(key, {static_cast<double>(section_index), static_cast<double>(field_index), static_cast<double>(spec.cache_hint)});\n";
  out << "      }\n";
  out << "    }\n";
  out << "    if (spec.with_dependencies && section_index + 1 < spec.depth) {\n";
  out << "      writer.dependency(section, std::string(\"section_\") + std::to_string(section_index + 1));\n";
  out << "    }\n";
  out << "  }\n";
  out << "  if (spec.with_conditionals) {\n";
  out << "    ManifestWriter nested;\n";
  out << "    nested.section(\"root\");\n";
  out << "    nested.field_string(\"conditional_marker\", std::string(spec.name) + \"_conditional\");\n";
  out << "    writer.section(\"root\");\n";
  out << "    writer.field_boolean(\"conditional_enabled\", true);\n";
  out << "    writer.conditional(\"conditional_enabled\", nested.bytes());\n";
  out << "  }\n";
  out << "  if (spec.with_schema) {\n";
  out << "    InternCache cache(64);\n";
  out << "    std::vector<SchemaField> fields;\n";
  out << "    fields.push_back({cache.intern(\"field_0_0\"), ValueType::String, false});\n";
  out << "    fields.push_back({cache.intern(\"conditional_enabled\"), ValueType::Boolean, false});\n";
  out << "    writer.schema(\"root\", fields);\n";
  out << "  }\n";
  out << "  return writer.bytes();\n";
  out << "}\n";
  out << "\n";
  out << "std::vector<uint8_t> build_scenario_manifest(std::size_t index) {\n";
  out << "  return build_scenario_manifest(scenario_at(index));\n";
  out << "}\n";
  out << "\n";
  out << "std::vector<std::string> scenario_names() {\n";
  out << "  std::vector<std::string> names;\n";
  out << "  names.reserve(scenario_count());\n";
  out << "  for (std::size_t i = 0; i < scenario_count(); ++i) {\n";
  out << "    names.push_back(kScenarioSpecs[i].name);\n";
  out << "  }\n";
  out << "  return names;\n";
  out << "}\n";
  out << "}\n";
  return 0;
}
