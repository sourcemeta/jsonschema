#ifndef SOURCEMETA_CORE_YAML_ROUNDTRIP_H_
#define SOURCEMETA_CORE_YAML_ROUNDTRIP_H_

#ifndef SOURCEMETA_CORE_YAML_EXPORT
#include <sourcemeta/core/yaml_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstdint>       // std::uint8_t, std::size_t
#include <optional>      // std::optional
#include <string>        // std::string
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup yaml
/// Holds per-node metadata collected during YAML parsing to reproduce the
/// original formatting
class SOURCEMETA_CORE_YAML_EXPORT YAMLRoundTrip {
public:
  /// The presentation style of a scalar value
  enum class ScalarStyle : std::uint8_t {
    /// The unquoted scalar style
    Plain,
    /// The single-quoted scalar style
    SingleQuoted,
    /// The double-quoted scalar style
    DoubleQuoted,
    /// The literal block scalar style that preserves line breaks
    Literal,
    /// The folded block scalar style that joins lines with spaces
    Folded
  };

  /// The presentation style of a collection
  enum class CollectionStyle : std::uint8_t {
    /// The indentation-based block collection style
    Block,
    /// The inline flow collection style delimited by brackets or braces
    Flow
  };

  /// The chomping behavior applied to a block scalar
  enum class Chomping : std::uint8_t {
    /// Keep a single trailing line break and drop the rest
    Clip,
    /// Remove all trailing line breaks
    Strip,
    /// Preserve every trailing line break
    Keep
  };

  /// Holds the formatting details recorded for a single node
  struct NodeStyle {
    /// The scalar presentation style
    std::optional<ScalarStyle> scalar;
    /// The collection presentation style
    std::optional<CollectionStyle> collection;
    /// The chomping behavior for a block scalar
    std::optional<Chomping> chomping;
    /// The explicit block scalar indentation width
    std::size_t explicit_indent{0};
    /// Whether the indentation indicator precedes the chomping indicator
    bool indent_before_chomping{false};
    /// The original block scalar content
    std::optional<std::string> block_content;
    /// The original plain scalar content
    std::optional<std::string> plain_content;
    /// The original quoted scalar content
    std::optional<std::string> quoted_content;
    /// The anchor name attached to the node
    std::optional<std::string> anchor;
    /// The comments preceding the node
    std::vector<std::string> comments_before;
    /// The comment on the same line as the node
    std::optional<std::string> comment_inline;
    /// The comment attached to a block scalar indicator
    std::optional<std::string> comment_on_indicator;
    /// Whether the flow collection uses compact formatting
    bool compact_flow{false};
  };

  /// The recorded formatting for each node by pointer
  std::unordered_map<Pointer, NodeStyle, Pointer::Hasher> styles;
  /// The alias name for each node referencing an anchor
  std::unordered_map<Pointer, std::string, Pointer::Hasher> aliases;
  /// The scalar style for each mapping key by pointer
  std::unordered_map<Pointer, ScalarStyle, Pointer::Hasher> key_styles;
  /// The original quoted content for each mapping key
  std::unordered_map<Pointer, std::string, Pointer::Hasher> key_quoted_contents;
  /// Whether the document begins with an explicit start marker
  bool explicit_document_start{false};
  /// Whether the document ends with an explicit end marker
  bool explicit_document_end{false};
  /// The comment on the document start marker
  std::optional<std::string> document_start_comment;
  /// The comment on the document end marker
  std::optional<std::string> document_end_comment;
  /// The comments preceding the document
  std::vector<std::string> leading_comments;
  /// The comments following the document start marker
  std::vector<std::string> post_start_comments;
  /// The comments preceding the document end marker
  std::vector<std::string> pre_end_comments;
  /// The comments following the document
  std::vector<std::string> trailing_comments;
  /// The indentation width used when emitting the document
  std::size_t indent_width{2};
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
