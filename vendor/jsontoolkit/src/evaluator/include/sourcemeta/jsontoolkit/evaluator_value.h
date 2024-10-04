#ifndef SOURCEMETA_JSONTOOLKIT_EVALUATOR_VALUE_H
#define SOURCEMETA_JSONTOOLKIT_EVALUATOR_VALUE_H

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <cstdint>       // std::uint8_t
#include <optional>      // std::optional, std::nullopt
#include <regex>         // std::regex
#include <set>           // std::set
#include <string>        // std::string
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <utility>       // std::pair
#include <vector>        // std::vector

namespace sourcemeta::jsontoolkit {

/// @ingroup evaluator
/// @brief Represents a compiler step empty value
struct SchemaCompilerValueNone {};

/// @ingroup evaluator
/// Represents a compiler step JSON value
using SchemaCompilerValueJSON = JSON;

// Note that for these steps, we prefer vectors over sets as the former performs
// better for small collections, where we can even guarantee uniqueness when
// generating the instructions

/// @ingroup evaluator
/// Represents a set of JSON values
using SchemaCompilerValueArray = std::vector<JSON>;

/// @ingroup evaluator
/// Represents a compiler step string values
using SchemaCompilerValueStrings = std::vector<JSON::String>;

/// @ingroup evaluator
/// Represents a compiler step JSON types value
using SchemaCompilerValueTypes = std::vector<JSON::Type>;

/// @ingroup evaluator
/// Represents a compiler step string value
using SchemaCompilerValueString = JSON::String;

/// @ingroup evaluator
/// Represents a compiler step JSON type value
using SchemaCompilerValueType = JSON::Type;

/// @ingroup evaluator
/// Represents a compiler step ECMA regular expression value. We store both the
/// original string and the regular expression as standard regular expressions
/// do not keep a copy of their original value (which we need for serialization
/// purposes)
using SchemaCompilerValueRegex = std::pair<std::regex, std::string>;

/// @ingroup evaluator
/// Represents a compiler step JSON unsigned integer value
using SchemaCompilerValueUnsignedInteger = std::size_t;

/// @ingroup evaluator
/// Represents a compiler step range value. The boolean option
/// modifies whether the range is considered exhaustively or
/// if the evaluator is allowed to break early
using SchemaCompilerValueRange =
    std::tuple<std::size_t, std::optional<std::size_t>, bool>;

/// @ingroup evaluator
/// Represents a compiler step boolean value
using SchemaCompilerValueBoolean = bool;

/// @ingroup evaluator
/// Represents a compiler step string to index map
using SchemaCompilerValueNamedIndexes =
    std::unordered_map<SchemaCompilerValueString,
                       SchemaCompilerValueUnsignedInteger>;

/// @ingroup evaluator
/// Represents a compiler step string logical type
enum class SchemaCompilerValueStringType : std::uint8_t { URI };

/// @ingroup evaluator
/// Represents an array loop compiler step annotation keywords
struct SchemaCompilerValueItemsAnnotationKeywords {
  const SchemaCompilerValueString index;
  const SchemaCompilerValueStrings filter;
  const SchemaCompilerValueStrings mask;
};

/// @ingroup evaluator
/// Represents an compiler step that maps strings to strings
using SchemaCompilerValueStringMap =
    std::unordered_map<SchemaCompilerValueString, SchemaCompilerValueStrings>;

/// @ingroup evaluator
/// Represents a compiler step JSON value accompanied with an index
using SchemaCompilerValueIndexedJSON =
    std::pair<SchemaCompilerValueUnsignedInteger, JSON>;

// Note that while we generally avoid sets, in this case, we want
// hash-based lookups on string collections that might get large.
/// @ingroup evaluator
/// Represents a compiler step value that consist of object property filters
using SchemaCompilerValuePropertyFilter =
    std::pair<std::set<SchemaCompilerValueString>,
              std::vector<SchemaCompilerValueRegex>>;

/// @ingroup evaluator
/// Represents a compiler step value that consists of two indexes
using SchemaCompilerValueIndexPair = std::pair<std::size_t, std::size_t>;

} // namespace sourcemeta::jsontoolkit

#endif
