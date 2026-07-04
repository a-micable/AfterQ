#pragma once
#include "cairn/manifest.h"
#include <cstdint>
#include <string>
#include <vector>

namespace cairn {
class ManifestWriter {
 public:
  void section(const std::string& name);
  void field_string(const std::string& key, const std::string& value);
  void field_integer(const std::string& key, int64_t value);
  void field_boolean(const std::string& key, bool value);
  void field_float_array(const std::string& key, const std::vector<double>& values);
  void include(const std::string& target);
  void conditional(const std::string& expression, const std::vector<uint8_t>& nested_manifest);
  void schema(const std::string& section, const std::vector<SchemaField>& fields);
  void dependency(const std::string& before, const std::string& after);
  const std::vector<uint8_t>& bytes() const;

 private:
  std::vector<uint8_t> out_;
  void record(RecordType type, const std::vector<uint8_t>& payload);
};

std::vector<uint8_t> make_minimal_manifest(const std::string& section);
}
