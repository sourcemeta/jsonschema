#ifndef SOURCEMETA_CORE_NUMERIC_ERROR_H
#define SOURCEMETA_CORE_NUMERIC_ERROR_H

#ifndef SOURCEMETA_CORE_NUMERIC_EXPORT
#include <sourcemeta/core/numeric_export.h>
#endif

#include <exception> // std::exception

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup numeric
/// This class represents a decimal parse error
class SOURCEMETA_CORE_NUMERIC_EXPORT DecimalParseError : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Invalid decimal string format";
  }
};

/// @ingroup numeric
/// This class represents a numeric division by zero error
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericDivisionByZeroError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Division by zero";
  }
};

/// @ingroup numeric
/// This class represents a numeric invalid operation error
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericInvalidOperationError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Invalid numeric operation";
  }
};

/// @ingroup numeric
/// This class represents a numeric overflow error
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericOverflowError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Numeric overflow";
  }
};

/// @ingroup numeric
/// This class represents a numeric out of memory error
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericOutOfMemoryError
    : public std::exception {
public:
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Out of memory";
  }
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
