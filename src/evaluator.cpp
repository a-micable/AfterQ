#include "cairn/evaluator.h"
#include <algorithm>
#include <sstream>

namespace cairn {

EvaluationResult Evaluator::evaluate(const ManifestNode& node) const {
  EvaluationResult result;
  apply_fields(node, result.values, result.errors);
  for (const ConditionalBlock& block : node.conditionals) {
    if (block.body && condition_true(block.expression.str(), result.values)) {
      apply_fields(*block.body, result.values, result.errors);
    }
  }
  const std::vector<std::string> schema_errors = validate(node);
  result.errors.insert(result.errors.end(), schema_errors.begin(), schema_errors.end());
  return result;
}

std::vector<std::string> Evaluator::validate(const ManifestNode& node) const {
  ConfigMap values;
  std::vector<std::string> ignored;
  apply_fields(node, values, ignored);
  std::vector<std::string> errors;
  for (const SchemaDecl& schema : node.schemas) {
    if (schema.section.str() != node.section.str() && schema.section.str() != "root") {
      continue;
    }
    for (const SchemaField& field : schema.fields) {
      const std::string key = field.key.str();
      const auto found = values.find(key);
      if (found == values.end()) {
        if (field.required) {
          errors.push_back("required field missing: " + key);
        }
        continue;
      }
      if (found->second.type != field.type) {
        errors.push_back("field type mismatch: " + key);
      }
    }
  }
  return errors;
}

void Evaluator::apply_fields(const ManifestNode& node, ConfigMap& values, std::vector<std::string>& errors) const {
  (void)errors;
  for (const Field& field : node.fields) {
    Value value = field.value;
    if (value.type == ValueType::String) {
      InternedString original = std::get<InternedString>(value.data);
      static InternCache substitution_cache(4096);
      value.data = substitution_cache.intern(substitute(original.str(), values));
    }
    values[field.key.str()] = value;
  }
}

bool Evaluator::condition_true(const std::string& expression, const ConfigMap& values) const {
  const std::size_t eq = expression.find("==");
  if (eq != std::string::npos) {
    const std::string left = expression.substr(0, eq);
    const std::string right = expression.substr(eq + 2u);
    const auto found = values.find(left);
    return found != values.end() && value_to_string(found->second) == right;
  }
  const auto found = values.find(expression);
  return found != values.end() && found->second.type == ValueType::Boolean && std::get<bool>(found->second.data);
}

std::string Evaluator::substitute(const std::string& input, const ConfigMap& values) const {
  std::string out;
  for (std::size_t i = 0; i < input.size();) {
    if (input[i] == '$' && i + 1u < input.size() && input[i + 1u] == '{') {
      const std::size_t end = input.find('}', i + 2u);
      if (end != std::string::npos) {
        const std::string key = input.substr(i + 2u, end - i - 2u);
        const auto found = values.find(key);
        if (found != values.end()) {
          out += value_to_string(found->second);
        }
        i = end + 1u;
        continue;
      }
    }
    out.push_back(input[i]);
    ++i;
  }
  return out;
}
}
