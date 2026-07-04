#pragma once
#include "cairn/manifest.h"
#include <string>
#include <vector>

namespace cairn {

struct EvaluationResult {
  ConfigMap values;
  std::vector<std::string> errors;
  bool ok() const { return errors.empty(); }
};

class Evaluator {
 public:
  EvaluationResult evaluate(const ManifestNode& node) const;
  std::vector<std::string> validate(const ManifestNode& node) const;

 private:
  void apply_fields(const ManifestNode& node, ConfigMap& values, std::vector<std::string>& errors) const;
  bool condition_true(const std::string& expression, const ConfigMap& values) const;
  std::string substitute(const std::string& input, const ConfigMap& values) const;
};
}
