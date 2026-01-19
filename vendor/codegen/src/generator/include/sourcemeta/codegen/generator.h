#ifndef SOURCEMETA_CODEGEN_GENERATOR_H_
#define SOURCEMETA_CODEGEN_GENERATOR_H_

#ifndef SOURCEMETA_CODEGEN_GENERATOR_EXPORT
#include <sourcemeta/codegen/generator_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/codegen/generator_typescript.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/codegen/ir.h>

#include <sourcemeta/core/jsonpointer.h>

#include <map>         // std::map
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <variant>     // std::visit
#include <vector>      // std::vector

/// @defgroup generator Generator
/// @brief The codegen JSON Schema code generation package

namespace sourcemeta::codegen {

/// @ingroup generator
SOURCEMETA_CODEGEN_GENERATOR_EXPORT
auto mangle(const std::string_view prefix,
            const sourcemeta::core::Pointer &pointer,
            const std::vector<std::string> &symbol,
            std::map<std::string, sourcemeta::core::Pointer> &cache)
    -> const std::string &;

/// @ingroup generator
template <typename T>
auto generate(std::ostream &output, const IRResult &result,
              const std::string_view prefix = "Schema") -> void {
  T visitor{output, prefix};
  const char *separator{""};
  for (const auto &entity : result) {
    output << separator;
    separator = "\n";
    std::visit(visitor, entity);
  }
}

} // namespace sourcemeta::codegen

#endif
