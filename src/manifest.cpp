#include "cairn/manifest.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace cairn {

std::string value_to_string(const Value& value) {
  switch (value.type) {
    case ValueType::String:
      return std::get<InternedString>(value.data).str();
    case ValueType::Integer:
      return std::to_string(std::get<int64_t>(value.data));
    case ValueType::Boolean:
      return std::get<bool>(value.data) ? "true" : "false";
    case ValueType::FloatArray: {
      std::ostringstream out;
      out << '[';
      const auto& values = std::get<std::vector<double>>(value.data);
      for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0u) {
          out << ',';
        }
        out << std::setprecision(12) << values[i];
      }
      out << ']';
      return out.str();
    }
  }
  throw std::runtime_error("unknown value type");
}

std::string value_type_name(ValueType type) {
  switch (type) {
    case ValueType::String:
      return "string";
    case ValueType::Integer:
      return "integer";
    case ValueType::Boolean:
      return "boolean";
    case ValueType::FloatArray:
      return "float_array";
  }
  throw std::runtime_error("unknown value type");
}

ValueType value_type_from_name(const std::string& name) {
  if (name == "string") return ValueType::String;
  if (name == "integer") return ValueType::Integer;
  if (name == "boolean") return ValueType::Boolean;
  if (name == "float_array") return ValueType::FloatArray;
  throw std::runtime_error("unknown value type name: " + name);
}

std::shared_ptr<ManifestNode> clone_node(const ManifestNode& node) {
  auto out = std::make_shared<ManifestNode>();
  out->section = node.section;
  out->fields = node.fields;
  out->includes = node.includes;
  for (const ConditionalBlock& block : node.conditionals) {
    ConditionalBlock copy;
    copy.expression = block.expression;
    copy.body = block.body ? clone_node(*block.body) : std::make_shared<ManifestNode>();
    out->conditionals.push_back(copy);
  }
  out->schemas = node.schemas;
  out->dependencies = node.dependencies;
  return out;
}

static void spaces(std::ostream& out, int indent) {
  for (int i = 0; i < indent; ++i) {
    out << ' ';
  }
}

void print_node(std::ostream& out, const ManifestNode& node, int indent) {
  spaces(out, indent);
  out << "section " << node.section.str() << '\n';
  for (const Field& field : node.fields) {
    spaces(out, indent + 2);
    out << "field " << field.key.str() << " = " << value_to_string(field.value) << '\n';
  }
  for (const InternedString& include : node.includes) {
    spaces(out, indent + 2);
    out << "include " << include.str() << '\n';
  }
  for (const SchemaDecl& schema : node.schemas) {
    spaces(out, indent + 2);
    out << "schema " << schema.section.str() << '\n';
    for (const SchemaField& field : schema.fields) {
      spaces(out, indent + 4);
      out << field.key.str() << ':' << value_type_name(field.type) << ':' << (field.required ? "required" : "optional") << '\n';
    }
  }
  for (const DependencyEdge& edge : node.dependencies) {
    spaces(out, indent + 2);
    out << "dependency " << edge.before.str() << " -> " << edge.after.str() << '\n';
  }
  for (const ConditionalBlock& block : node.conditionals) {
    spaces(out, indent + 2);
    out << "if " << block.expression.str() << '\n';
    if (block.body) {
      print_node(out, *block.body, indent + 4);
    }
  }
}
}
