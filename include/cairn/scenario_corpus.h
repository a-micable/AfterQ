#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace cairn {

struct ScenarioSpec {
  const char* name;
  int depth;
  int fields_per_section;
  bool with_schema;
  bool with_conditionals;
  bool with_dependencies;
  int cache_hint;
};

std::size_t scenario_count();
const ScenarioSpec& scenario_at(std::size_t index);
std::vector<uint8_t> build_scenario_manifest(const ScenarioSpec& spec);
std::vector<uint8_t> build_scenario_manifest(std::size_t index);
std::vector<std::string> scenario_names();

}
