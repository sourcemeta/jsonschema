#ifndef SOURCEMETA_BLAZE_CODEGEN_H_
#define SOURCEMETA_BLAZE_CODEGEN_H_

#ifndef SOURCEMETA_BLAZE_CODEGEN_EXPORT
#include <sourcemeta/blaze/codegen_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/blaze/codegen_error.h>
#include <sourcemeta/blaze/codegen_typescript.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <map>         // std::map
#include <optional>    // std::optional, std::nullopt
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <variant>     // std::variant, std::visit
#include <vector>      // std::vector

/// @defgroup codegen Codegen
/// @brief A code generation utility built on top of Blaze

namespace sourcemeta::blaze {

/// @ingroup codegen
enum class CodegenIRScalarType : std::uint8_t {
  String,
  Number,
  Integer,
  Boolean,
  Null
};

/// @ingroup codegen
struct CodegenIRType {
  sourcemeta::core::Pointer pointer;
  std::vector<std::string> symbol;
};

/// @ingroup codegen
struct CodegenIRScalar : CodegenIRType {
  CodegenIRScalarType value;
};

/// @ingroup codegen
struct CodegenIREnumeration : CodegenIRType {
  std::vector<sourcemeta::core::JSON> values;
};

/// @ingroup codegen
struct CodegenIRUnion : CodegenIRType {
  std::vector<CodegenIRType> values;
};

/// @ingroup codegen
struct CodegenIRIntersection : CodegenIRType {
  std::vector<CodegenIRType> values;
};

/// @ingroup codegen
struct CodegenIRObjectValue : CodegenIRType {
  bool required;
  bool immutable;
};

/// @ingroup codegen
struct CodegenIRObjectPatternProperty : CodegenIRType {
  std::optional<std::string> prefix;
};

/// @ingroup codegen
struct CodegenIRObject : CodegenIRType {
  // To preserve the user's ordering
  std::vector<std::pair<sourcemeta::core::JSON::String, CodegenIRObjectValue>>
      members;
  std::variant<bool, CodegenIRType> additional;
  std::vector<CodegenIRObjectPatternProperty> pattern;
};

/// @ingroup codegen
struct CodegenIRArray : CodegenIRType {
  std::optional<CodegenIRType> items;
};

/// @ingroup codegen
struct CodegenIRTuple : CodegenIRType {
  std::vector<CodegenIRType> items;
  std::optional<CodegenIRType> additional;
};

/// @ingroup codegen
struct CodegenIRImpossible : CodegenIRType {};

/// @ingroup codegen
struct CodegenIRAny : CodegenIRType {};

/// @ingroup codegen
struct CodegenIRConditional : CodegenIRType {
  CodegenIRType condition;
  CodegenIRType consequent;
  CodegenIRType alternative;
};

/// @ingroup codegen
struct CodegenIRReference : CodegenIRType {
  CodegenIRType target;
};

/// @ingroup codegen
using CodegenIREntity =
    std::variant<CodegenIRObject, CodegenIRScalar, CodegenIREnumeration,
                 CodegenIRUnion, CodegenIRIntersection, CodegenIRConditional,
                 CodegenIRArray, CodegenIRTuple, CodegenIRImpossible,
                 CodegenIRAny, CodegenIRReference>;

/// @ingroup codegen
using CodegenIRResult = std::vector<CodegenIREntity>;

/// @ingroup codegen
using CodegenCompiler = std::function<CodegenIREntity(
    const sourcemeta::core::JSON &, const sourcemeta::core::SchemaFrame &,
    const sourcemeta::core::SchemaFrame::Location &,
    const sourcemeta::core::SchemaResolver &, const sourcemeta::core::JSON &)>;

/// @ingroup codegen
SOURCEMETA_BLAZE_CODEGEN_EXPORT
auto default_compiler(const sourcemeta::core::JSON &schema,
                      const sourcemeta::core::SchemaFrame &frame,
                      const sourcemeta::core::SchemaFrame::Location &location,
                      const sourcemeta::core::SchemaResolver &resolver,
                      const sourcemeta::core::JSON &subschema)
    -> CodegenIREntity;

/// @ingroup codegen
SOURCEMETA_BLAZE_CODEGEN_EXPORT
auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const CodegenCompiler &compiler,
             const std::string_view default_dialect = "",
             const std::string_view default_id = "") -> CodegenIRResult;

/// @ingroup codegen
SOURCEMETA_BLAZE_CODEGEN_EXPORT
auto symbol(const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location)
    -> std::vector<std::string>;

/// @ingroup codegen
SOURCEMETA_BLAZE_CODEGEN_EXPORT
auto mangle(const std::string_view prefix,
            const sourcemeta::core::Pointer &pointer,
            const std::vector<std::string> &symbol,
            std::map<std::string, sourcemeta::core::Pointer> &cache)
    -> const std::string &;

/// @ingroup codegen
template <typename T>
auto generate(std::ostream &output, const CodegenIRResult &result,
              const std::string_view prefix = "Schema") -> void {
  T visitor{output, prefix};
  const char *separator{""};
  for (const auto &entity : result) {
    output << separator;
    separator = "\n";
    std::visit(visitor, entity);
  }
}

} // namespace sourcemeta::blaze

#endif
