#ifndef SOURCEMETA_CORE_JSONPOINTER_POSITION_H_
#define SOURCEMETA_CORE_JSONPOINTER_POSITION_H_

#ifndef SOURCEMETA_CORE_JSONPOINTER_EXPORT
#include <sourcemeta/core/jsonpointer_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <sourcemeta/core/jsonpointer_pointer.h>

#include <cstddef>  // std::size_t
#include <cstdint>  // std::uint64_t
#include <map>      // std::map
#include <optional> // std::optional
#include <stack>    // std::stack
#include <tuple>    // std::tuple
#include <utility>  // std::pair

namespace sourcemeta::core {

/// @ingroup jsonpointer
/// A convenient parsing callback to obtain JSON document line/column
/// information using JSON Pointer. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
///
/// const auto input{"{\n  \"foo\": \"bar\"\n}"};;
/// sourcemeta::core::PointerPositionTracker tracker;
/// sourcemeta::core::parse_json(stream, std::ref(tracker));
/// assert(tracker.size() == 2);
/// const auto foo{tracker.get(sourcemeta::core::Pointer{"foo"})};
/// assert(foo.has_value());
///
/// // Start line and column
/// assert(std::get<0>(foo.value()) == 2);
/// assert(std::get<1>(foo.value()) == 10);
///
/// // End line and column
/// assert(std::get<3>(foo.value()) == 2);
/// assert(std::get<4>(foo.value()) == 14);
/// ```
class SOURCEMETA_CORE_JSONPOINTER_EXPORT PointerPositionTracker {
public:
  using Pointer = GenericPointer<JSON::String, PropertyHashJSON<JSON::String>>;
  using Position =
      std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t>;
  auto operator()(const JSON::ParsePhase phase, const JSON::Type,
                  const std::uint64_t line, const std::uint64_t column,
                  const JSON::ParseContext context, const std::size_t index,
                  const JSON::StringView property) -> void;
  [[nodiscard]] auto get(const Pointer &pointer) const
      -> std::optional<Position>;
  [[nodiscard]] auto size() const -> std::size_t;
  [[nodiscard]] auto to_json() const -> JSON;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  Pointer current;
  std::stack<std::pair<std::uint64_t, std::uint64_t>> stack;
  std::map<Pointer, Position> data;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
