#pragma once
#include "cairn/intern_cache.h"
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace cairn {

enum class RecordType : uint16_t {
  Section = 1,
  Field = 2,
  Include = 3,
  Conditional = 4,
  Schema = 5,
  Dependency = 6
};

enum class ValueType : uint8_t {
  String = 1,
  Integer = 2,
  Boolean = 3,
  FloatArray = 4
};

struct Value {
  ValueType type = ValueType::String;
  std::variant<InternedString, int64_t, bool, std::vector<double>> data;
};

struct Field {
  InternedString key;
  Value value;
};

struct SchemaField {
  InternedString key;
  ValueType type = ValueType::String;
  bool required = true;
};

struct SchemaDecl {
  InternedString section;
  std::vector<SchemaField> fields;
};

struct DependencyEdge {
  InternedString before;
  InternedString after;
};

struct ConditionalBlock {
  InternedString expression;
  std::shared_ptr<struct ManifestNode> body;
};

struct ManifestNode {
  InternedString section;
  std::vector<Field> fields;
  std::vector<InternedString> includes;
  std::vector<ConditionalBlock> conditionals;
  std::vector<SchemaDecl> schemas;
  std::vector<DependencyEdge> dependencies;
};

using ConfigMap = std::map<std::string, Value>;

std::string value_to_string(const Value& value);
std::string value_type_name(ValueType type);
ValueType value_type_from_name(const std::string& name);
std::shared_ptr<ManifestNode> clone_node(const ManifestNode& node);
void print_node(std::ostream& out, const ManifestNode& node, int indent = 0);
}
