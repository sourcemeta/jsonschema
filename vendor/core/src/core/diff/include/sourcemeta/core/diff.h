#ifndef SOURCEMETA_CORE_DIFF_H_
#define SOURCEMETA_CORE_DIFF_H_

#ifndef SOURCEMETA_CORE_DIFF_EXPORT
#include <sourcemeta/core/diff_export.h>
#endif

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <ostream>     // std::ostream
#include <string_view> // std::string_view
#include <vector>      // std::vector

/// @defgroup diff Diff
/// @brief A line-oriented textual difference implementation
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/diff.h>
/// ```

namespace sourcemeta::core {

/// @ingroup diff
///
/// The result of comparing two inputs, holding the tokens of each alongside the
/// operations that relate them, plus the vocabulary that describes how a
/// comparison is performed and rendered.
///
/// The tokens are views into the inputs that were compared, which must outlive
/// this result.
struct Diff {
  /// The granularity at which inputs are compared
  enum class Mode : std::uint8_t {
    /// Compare newline-delimited lines
    Line
  };

  /// The strategy used to locate the common subsequences of two inputs
  enum class Algorithm : std::uint8_t {
    /// A greedy shortest edit script search over the edit graph, as described
    /// by Eugene W. Myers in "An O(ND) Difference Algorithm and Its
    /// Variations", Algorithmica Vol. 1, 1986, pp. 251-266
    /// (http://www.xmailserver.org/diff2.pdf)
    Myers
  };

  /// The textual serialisation used to render a set of differences
  enum class Format : std::uint8_t {
    /// The unified format, as standardised by POSIX in IEEE Std 1003.1-2024
    /// (https://pubs.opengroup.org/onlinepubs/9799919799/utilities/diff.html)
    Unified
  };

  /// A single contiguous transformation between two inputs, expressed as a
  /// half-open range into the tokens of each of them.
  ///
  /// A deletion carries an empty modified range that marks where in the
  /// modified input the removal occurred, and an insertion carries an empty
  /// original range that marks where in the original input the addition
  /// occurred.
  struct Operation {
    /// The kind of transformation that a difference operation represents
    enum class Type : std::uint8_t {
      /// The spans are present in both inputs
      Equal,
      /// The span is only present in the original input
      Delete,
      /// The span is only present in the modified input
      Insert
    };

    /// The kind of transformation
    Type type;
    /// The index of the first affected token of the original input
    std::size_t original_start;
    /// The index one past the last affected token of the original input
    std::size_t original_end;
    /// The index of the first affected token of the modified input
    std::size_t modified_start;
    /// The index one past the last affected token of the modified input
    std::size_t modified_end;

    auto operator==(const Operation &) const noexcept -> bool = default;
  };

  /// Controls the presentation of a rendered set of differences
  struct FormatOptions {
    /// The name given to the original input in the header
    std::string_view original_label{"a"};
    /// The name given to the modified input in the header
    std::string_view modified_label{"b"};
    /// The number of unchanged lines shown around each change
    std::size_t context{3};
  };

  /// The tokens of the original input
  std::vector<std::string_view> original;
  /// The tokens of the modified input
  std::vector<std::string_view> modified;
  /// The transformations that turn the original input into the modified one
  std::vector<Operation> operations;
  /// Whether the original input ended with a line terminator
  bool original_ends_with_newline{true};
  /// Whether the modified input ended with a line terminator
  bool modified_ends_with_newline{true};
};

/// @ingroup diff
///
/// Compute the differences between two inputs. For example:
///
/// ```cpp
/// #include <sourcemeta/core/diff.h>
/// #include <cassert>
/// #include <vector>
///
/// const auto result{sourcemeta::core::diff(
///     "foo\nbar\n", "foo\nbaz\n", sourcemeta::core::Diff::Mode::Line,
///     sourcemeta::core::Diff::Algorithm::Myers)};
///
/// assert((result.operations ==
///         std::vector<sourcemeta::core::Diff::Operation>{
///             {sourcemeta::core::Diff::Operation::Type::Equal, 0, 1, 0, 1},
///             {sourcemeta::core::Diff::Operation::Type::Delete, 1, 2, 1, 1},
///             {sourcemeta::core::Diff::Operation::Type::Insert, 2, 2, 1,
///             2}}));
/// ```
///
/// The tokens of the result are views into the given inputs, which must
/// outlive it.
SOURCEMETA_CORE_DIFF_EXPORT
auto diff(const std::string_view original, const std::string_view modified,
          const Diff::Mode mode, const Diff::Algorithm algorithm) -> Diff;

/// @ingroup diff
///
/// Render a set of differences into a given C++ standard output stream. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/diff.h>
/// #include <cassert>
/// #include <sstream>
///
/// const auto result{sourcemeta::core::diff(
///     "foo\nbar\n", "foo\nbaz\n", sourcemeta::core::Diff::Mode::Line,
///     sourcemeta::core::Diff::Algorithm::Myers)};
///
/// std::ostringstream stream;
/// sourcemeta::core::stringify(result, stream,
///                             sourcemeta::core::Diff::Format::Unified);
///
/// assert(stream.str() == "--- a\n"
///                        "+++ b\n"
///                        "@@ -1,2 +1,2 @@\n"
///                        " foo\n"
///                        "-bar\n"
///                        "+baz\n");
/// ```
SOURCEMETA_CORE_DIFF_EXPORT
auto stringify(const Diff &document, std::ostream &stream,
               const Diff::Format format,
               const Diff::FormatOptions &options = {}) -> void;

} // namespace sourcemeta::core

#endif
