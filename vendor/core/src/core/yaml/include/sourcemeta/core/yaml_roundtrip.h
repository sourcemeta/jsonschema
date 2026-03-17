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
  enum class ScalarStyle : std::uint8_t {
    Plain,
    SingleQuoted,
    DoubleQuoted,
    Literal,
    Folded
  };

  enum class CollectionStyle : std::uint8_t { Block, Flow };

  enum class Chomping : std::uint8_t { Clip, Strip, Keep };

  struct NodeStyle {
    std::optional<ScalarStyle> scalar;
    std::optional<CollectionStyle> collection;
    std::optional<Chomping> chomping;
    std::size_t explicit_indent{0};
    bool indent_before_chomping{false};
    std::optional<std::string> block_content;
    std::optional<std::string> plain_content;
    std::optional<std::string> quoted_content;
    std::optional<std::string> anchor;
    std::vector<std::string> comments_before;
    std::optional<std::string> comment_inline;
    std::optional<std::string> comment_on_indicator;
    bool compact_flow{false};
  };

  std::unordered_map<Pointer, NodeStyle, Pointer::Hasher> styles;
  std::unordered_map<Pointer, std::string, Pointer::Hasher> aliases;
  std::unordered_map<Pointer, ScalarStyle, Pointer::Hasher> key_styles;
  std::unordered_map<Pointer, std::string, Pointer::Hasher> key_quoted_contents;
  bool explicit_document_start{false};
  bool explicit_document_end{false};
  std::optional<std::string> document_start_comment;
  std::optional<std::string> document_end_comment;
  std::vector<std::string> leading_comments;
  std::vector<std::string> post_start_comments;
  std::vector<std::string> pre_end_comments;
  std::vector<std::string> trailing_comments;
  std::size_t indent_width{2};
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
