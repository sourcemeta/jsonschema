#ifndef SOURCEMETA_CODEGEN_IR_H_
#define SOURCEMETA_CODEGEN_IR_H_

#ifndef SOURCEMETA_CODEGEN_IR_EXPORT
#include <sourcemeta/codegen/ir_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/codegen/ir_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdint>    // std::uint8_t
#include <functional> // std::function
#include <optional>   // std::optional, std::nullopt
#include <string>     // std::string
#include <utility>    // std::pair
#include <variant>    // std::variant
#include <vector>     // std::vector

/// @defgroup ir IR
/// @brief The codegen JSON Schema intermediary format

namespace sourcemeta::codegen {

/// @ingroup ir
enum class IRScalarType : std::uint8_t {
  String,
  Number,
  Integer,
  Boolean,
  Null
};

/// @ingroup ir
struct IRType {
  sourcemeta::core::Pointer pointer;
  std::vector<std::string> symbol;
};

/// @ingroup ir
struct IRScalar : IRType {
  IRScalarType value;
};

/// @ingroup ir
struct IREnumeration : IRType {
  std::vector<sourcemeta::core::JSON> values;
};

/// @ingroup ir
struct IRUnion : IRType {
  std::vector<IRType> values;
};

/// @ingroup ir
struct IRObjectValue : IRType {
  bool required;
  bool immutable;
};

/// @ingroup ir
struct IRObject : IRType {
  // To preserve the user's ordering
  std::vector<std::pair<sourcemeta::core::JSON::String, IRObjectValue>> members;
  std::variant<bool, IRType> additional;
};

/// @ingroup ir
struct IRArray : IRType {
  std::optional<IRType> items;
};

/// @ingroup ir
struct IRTuple : IRType {
  std::vector<IRType> items;
  std::optional<IRType> additional;
};

/// @ingroup ir
struct IRImpossible : IRType {};

/// @ingroup ir
struct IRReference : IRType {
  IRType target;
};

/// @ingroup ir
using IREntity = std::variant<IRObject, IRScalar, IREnumeration, IRUnion,
                              IRArray, IRTuple, IRImpossible, IRReference>;

/// @ingroup ir
using IRResult = std::vector<IREntity>;

/// @ingroup ir
using Compiler = std::function<IREntity(
    const sourcemeta::core::JSON &, const sourcemeta::core::SchemaFrame &,
    const sourcemeta::core::SchemaFrame::Location &,
    const sourcemeta::core::SchemaResolver &, const sourcemeta::core::JSON &)>;

/// @ingroup ir
SOURCEMETA_CODEGEN_IR_EXPORT
auto default_compiler(const sourcemeta::core::JSON &schema,
                      const sourcemeta::core::SchemaFrame &frame,
                      const sourcemeta::core::SchemaFrame::Location &location,
                      const sourcemeta::core::SchemaResolver &resolver,
                      const sourcemeta::core::JSON &subschema) -> IREntity;

/// @ingroup ir
SOURCEMETA_CODEGEN_IR_EXPORT
auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler,
             const std::string_view default_dialect = "",
             const std::string_view default_id = "") -> IRResult;

/// @ingroup ir
SOURCEMETA_CODEGEN_IR_EXPORT
auto symbol(const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location)
    -> std::vector<std::string>;

} // namespace sourcemeta::codegen

#endif
