#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_H_

/// @defgroup runtime Runtime
/// @brief The encoder/decoder parts of JSON BinPack
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/jsonbinpack/runtime.h>
/// ```

#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT
#include <sourcemeta/jsonbinpack/runtime_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <sourcemeta/jsonbinpack/runtime_decoder.h>
#include <sourcemeta/jsonbinpack/runtime_encoder.h>
#include <sourcemeta/jsonbinpack/runtime_encoding.h>

#include <exception> // std::exception
#include <utility>   // std::move

namespace sourcemeta::jsonbinpack {

/// @ingroup runtime
SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT
auto load(const sourcemeta::core::JSON &input) -> Encoding;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup runtime
/// This class represents an encoding error
class SOURCEMETA_JSONBINPACK_RUNTIME_EXPORT EncodingError
    : public std::exception {
public:
  EncodingError(sourcemeta::core::JSON::String message)
      : message_{std::move(message)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

private:
  const sourcemeta::core::JSON::String message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::jsonbinpack

#endif
