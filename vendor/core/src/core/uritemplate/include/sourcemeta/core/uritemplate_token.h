#ifndef SOURCEMETA_CORE_URITEMPLATE_TOKEN_H_
#define SOURCEMETA_CORE_URITEMPLATE_TOKEN_H_

#ifndef SOURCEMETA_CORE_URITEMPLATE_EXPORT
#include <sourcemeta/core/uritemplate_export.h>
#endif

#include <cstdint>     // std::uint16_t
#include <string_view> // std::string_view
#include <variant>     // std::variant
#include <vector>      // std::vector

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/// @ingroup uritemplate
/// A literal string segment in a URI Template
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateTokenLiteral {
  /// The literal string content
  std::string_view value;
};

/// @ingroup uritemplate
/// A variable specification within a URI Template expression
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateVariableSpecification {
  /// The variable name
  std::string_view name;
  // As per the RFC, the range is 1-9999. 0 means "no prefix length"
  /// The maximum prefix length to apply to the value
  std::uint16_t length{0};
  /// Whether the variable is exploded into multiple values
  bool explode{false};
};

/// @ingroup uritemplate
/// A simple string variable expansion {var} in a URI Template (Level 1)
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateTokenVariable {
  /// The variable specifications to expand
  std::vector<URITemplateVariableSpecification> variables;
  /// The character that separates expanded values
  static constexpr char separator = ',';
  /// Whether the expansion includes variable names
  static constexpr bool named = false;
  /// Whether reserved characters are preserved unencoded
  static constexpr bool allow_reserved = false;
};

/// @ingroup uritemplate
/// A reserved expansion {+var} in a URI Template (Level 2)
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateTokenReservedExpansion {
  /// The variable specifications to expand
  std::vector<URITemplateVariableSpecification> variables;
  /// The operator character that introduces the expression
  static constexpr char op = '+';
  /// The character that separates expanded values
  static constexpr char separator = ',';
  /// Whether the expansion includes variable names
  static constexpr bool named = false;
  /// Whether reserved characters are preserved unencoded
  static constexpr bool allow_reserved = true;
};

/// @ingroup uritemplate
/// A fragment expansion {\#var} in a URI Template (Level 2)
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateTokenFragmentExpansion {
  /// The variable specifications to expand
  std::vector<URITemplateVariableSpecification> variables;
  /// The operator character that introduces the expression
  static constexpr char op = '#';
  /// The character that separates expanded values
  static constexpr char separator = ',';
  /// The character prepended to the expansion output
  static constexpr char prefix = '#';
  /// Whether the expansion includes variable names
  static constexpr bool named = false;
  /// Whether reserved characters are preserved unencoded
  static constexpr bool allow_reserved = true;
};

/// @ingroup uritemplate
/// A label expansion {.var} in a URI Template (Level 3)
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateTokenLabelExpansion {
  /// The variable specifications to expand
  std::vector<URITemplateVariableSpecification> variables;
  /// The operator character that introduces the expression
  static constexpr char op = '.';
  /// The character that separates expanded values
  static constexpr char separator = '.';
  /// The character prepended to the expansion output
  static constexpr char prefix = '.';
  /// Whether the expansion includes variable names
  static constexpr bool named = false;
  /// Whether reserved characters are preserved unencoded
  static constexpr bool allow_reserved = false;
};

/// @ingroup uritemplate
/// A path expansion {/var} in a URI Template (Level 3)
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateTokenPathExpansion {
  /// The variable specifications to expand
  std::vector<URITemplateVariableSpecification> variables;
  /// The operator character that introduces the expression
  static constexpr char op = '/';
  /// The character that separates expanded values
  static constexpr char separator = '/';
  /// The character prepended to the expansion output
  static constexpr char prefix = '/';
  /// Whether the expansion includes variable names
  static constexpr bool named = false;
  /// Whether reserved characters are preserved unencoded
  static constexpr bool allow_reserved = false;
};

/// @ingroup uritemplate
/// A path parameter expansion {;var} in a URI Template (Level 3)
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT
    URITemplateTokenPathParameterExpansion {
  /// The variable specifications to expand
  std::vector<URITemplateVariableSpecification> variables;
  /// The operator character that introduces the expression
  static constexpr char op = ';';
  /// The character that separates expanded values
  static constexpr char separator = ';';
  /// The character prepended to the expansion output
  static constexpr char prefix = ';';
  /// Whether the expansion includes variable names
  static constexpr bool named = true;
  /// Whether reserved characters are preserved unencoded
  static constexpr bool allow_reserved = false;
};

/// @ingroup uritemplate
/// A query expansion {?var} in a URI Template (Level 3)
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateTokenQueryExpansion {
  /// The variable specifications to expand
  std::vector<URITemplateVariableSpecification> variables;
  /// The operator character that introduces the expression
  static constexpr char op = '?';
  /// The character that separates expanded values
  static constexpr char separator = '&';
  /// The character prepended to the expansion output
  static constexpr char prefix = '?';
  /// Whether the expansion includes variable names
  static constexpr bool named = true;
  /// Whether reserved characters are preserved unencoded
  static constexpr bool allow_reserved = false;
  /// The character appended to names with empty values
  static constexpr char empty_suffix = '=';
};

/// @ingroup uritemplate
/// A query continuation expansion {&var} in a URI Template (Level 3)
struct SOURCEMETA_CORE_URITEMPLATE_EXPORT
    URITemplateTokenQueryContinuationExpansion {
  /// The variable specifications to expand
  std::vector<URITemplateVariableSpecification> variables;
  /// The operator character that introduces the expression
  static constexpr char op = '&';
  /// The character that separates expanded values
  static constexpr char separator = '&';
  /// The character prepended to the expansion output
  static constexpr char prefix = '&';
  /// Whether the expansion includes variable names
  static constexpr bool named = true;
  /// Whether reserved characters are preserved unencoded
  static constexpr bool allow_reserved = false;
  /// The character appended to names with empty values
  static constexpr char empty_suffix = '=';
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/// @ingroup uritemplate
/// A token in a parsed URI Template
using URITemplateToken = std::variant<
    URITemplateTokenLiteral, URITemplateTokenVariable,
    URITemplateTokenReservedExpansion, URITemplateTokenFragmentExpansion,
    URITemplateTokenLabelExpansion, URITemplateTokenPathExpansion,
    URITemplateTokenPathParameterExpansion, URITemplateTokenQueryExpansion,
    URITemplateTokenQueryContinuationExpansion>;

} // namespace sourcemeta::core

#endif
