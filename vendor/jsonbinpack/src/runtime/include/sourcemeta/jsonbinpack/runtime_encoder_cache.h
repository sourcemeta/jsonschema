#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_ENCODER_CACHE_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_ENCODER_CACHE_H_
#ifndef DOXYGEN

#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT
#include <sourcemeta/jsonbinpack/runtime_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <functional> // std::reference_wrapper
#include <map>        // std::map
#include <optional>   // std::optional
#include <utility>    // std::pair

namespace sourcemeta::jsonbinpack {

class SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT Cache {
public:
  enum class Type { Standalone, PrefixLengthVarintPlusOne };
  auto record(const sourcemeta::core::JSON::String &value,
              const std::uint64_t offset, const Type type) -> void;
  auto find(const sourcemeta::core::JSON::String &value, const Type type) const
      -> std::optional<std::uint64_t>;

#ifndef DOXYGEN
  // This method is considered private. We only expose it for testing purposes
  auto remove_oldest() -> void;
#endif

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  std::uint64_t byte_size{0};
  using Entry = std::pair<sourcemeta::core::JSON::String, Type>;
  std::map<Entry, std::uint64_t> data;
  std::map<std::uint64_t, std::reference_wrapper<const Entry>> order;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
};

} // namespace sourcemeta::jsonbinpack

#endif
#endif
