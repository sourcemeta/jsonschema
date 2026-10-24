#ifndef SOURCEMETA_BLAZE_BINPACK_ERROR_H_
#define SOURCEMETA_BLAZE_BINPACK_ERROR_H_

#ifndef SOURCEMETA_BLAZE_BINPACK_EXPORT
#include <sourcemeta/blaze/binpack_export.h>
#endif

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::blaze {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup binpack
/// An error that represents an encoding that the runtime does not implement
class SOURCEMETA_BLAZE_BINPACK_EXPORT BinPackEncodingError
    : public std::exception {
public:
  BinPackEncodingError(std::string name) : name_{std::move(name)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The encoding is not recognized";
  }

  /// The name of the encoding that could not be recognized
  [[nodiscard]] auto name() const noexcept -> std::string_view {
    return this->name_;
  }

private:
  std::string name_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
