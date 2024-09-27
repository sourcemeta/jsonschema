#ifndef SOURCEMETA_JSONTOOLKIT_EVALUATOR_ERROR_H
#define SOURCEMETA_JSONTOOLKIT_EVALUATOR_ERROR_H

#include "evaluator_export.h"

#include <exception> // std::exception
#include <string>    // std::string
#include <utility>   // std::move

namespace sourcemeta::jsontoolkit {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup jsonschema
/// An error that represents a schema evaluation error event
class SOURCEMETA_JSONTOOLKIT_EVALUATOR_EXPORT SchemaEvaluationError
    : public std::exception {
public:
  SchemaEvaluationError(std::string message) : message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

private:
  std::string message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::jsontoolkit

#endif
