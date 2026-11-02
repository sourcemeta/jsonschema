#ifndef SOURCEMETA_CORE_NUMERIC_ERROR_H
#define SOURCEMETA_CORE_NUMERIC_ERROR_H

#ifndef SOURCEMETA_CORE_NUMERIC_EXPORT
#include <sourcemeta/core/numeric_export.h>
#endif

#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup numeric
/// This class represents a decimal parse error
class SOURCEMETA_CORE_NUMERIC_EXPORT DecimalParseError : public Error {
public:
  DecimalParseError() noexcept : Error{"Invalid decimal string format"} {}
};

/// @ingroup numeric
/// This class represents a variable-length integer parse error
class SOURCEMETA_CORE_NUMERIC_EXPORT VarintParseError : public Error {
public:
  VarintParseError() noexcept : Error{"Invalid variable-length integer"} {}
};

/// @ingroup numeric
/// This class represents a numeric division by zero error
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericDivisionByZeroError : public Error {
public:
  NumericDivisionByZeroError() noexcept : Error{"Division by zero"} {}
};

/// @ingroup numeric
/// This class represents a numeric invalid operation error
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericInvalidOperationError
    : public Error {
public:
  NumericInvalidOperationError() noexcept
      : Error{"Invalid numeric operation"} {}
};

/// @ingroup numeric
/// This class represents a numeric overflow error
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericOverflowError : public Error {
public:
  NumericOverflowError() noexcept : Error{"Numeric overflow"} {}
};

/// @ingroup numeric
/// This class represents a numeric value that falls outside the range of the
/// target type
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericOutOfRangeError : public Error {
public:
  NumericOutOfRangeError() noexcept : Error{"Numeric value is out of range"} {}
};

/// @ingroup numeric
/// This class represents a numeric out of memory error
class SOURCEMETA_CORE_NUMERIC_EXPORT NumericOutOfMemoryError : public Error {
public:
  NumericOutOfMemoryError() noexcept : Error{"Out of memory"} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
