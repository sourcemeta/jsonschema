#ifndef SOURCEMETA_JSONSCHEMA_CLI_COMPATIBILITY_H_
#define SOURCEMETA_JSONSCHEMA_CLI_COMPATIBILITY_H_

#include <sourcemeta/core/json.h>

#include <string>
#include <vector>

namespace sourcemeta::jsonschema {

enum class CompatibilitySeverity { Breaking, Warning, Safe };

struct CompatibilityChange {
  CompatibilitySeverity severity;
  std::string kind;
  std::string path;
  std::string message;
};

struct CompatibilityReport {
  std::vector<CompatibilityChange> breaking;
  std::vector<CompatibilityChange> warnings;
  std::vector<CompatibilityChange> safe;

  [[nodiscard]] auto empty() const noexcept -> bool;
  [[nodiscard]] auto has_breaking_changes() const noexcept -> bool;
  [[nodiscard]] auto to_json() const -> sourcemeta::core::JSON;
};

class CompatibilityChecker {
public:
  [[nodiscard]] auto compare(const sourcemeta::core::JSON &old_schema,
                             const sourcemeta::core::JSON &new_schema) const
      -> CompatibilityReport;
};

} // namespace sourcemeta::jsonschema

#endif
