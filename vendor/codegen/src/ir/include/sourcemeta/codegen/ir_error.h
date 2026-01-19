#ifndef SOURCEMETA_CODEGEN_IR_ERROR_H_
#define SOURCEMETA_CODEGEN_IR_ERROR_H_

#ifndef SOURCEMETA_CODEGEN_IR_EXPORT
#include <sourcemeta/codegen/ir_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::codegen {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup ir
/// An error that represents an unsupported keyword during IR compilation
class SOURCEMETA_CODEGEN_IR_EXPORT UnsupportedKeywordError
    : public std::exception {
public:
  UnsupportedKeywordError(sourcemeta::core::JSON json,
                          sourcemeta::core::Pointer pointer,
                          std::string keyword, const char *message)
      : json_{std::move(json)}, pointer_{std::move(pointer)},
        keyword_{std::move(keyword)}, message_{message} {}
  UnsupportedKeywordError(sourcemeta::core::JSON json,
                          const sourcemeta::core::WeakPointer &pointer,
                          std::string keyword, const char *message)
      : UnsupportedKeywordError{std::move(json),
                                sourcemeta::core::to_pointer(pointer),
                                std::move(keyword), message} {}
  UnsupportedKeywordError(sourcemeta::core::JSON json,
                          sourcemeta::core::Pointer pointer,
                          std::string keyword, std::string message) = delete;
  UnsupportedKeywordError(sourcemeta::core::JSON json,
                          sourcemeta::core::Pointer pointer,
                          std::string keyword, std::string &&message) = delete;
  UnsupportedKeywordError(sourcemeta::core::JSON json,
                          sourcemeta::core::Pointer pointer,
                          std::string keyword,
                          std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto json() const noexcept -> const sourcemeta::core::JSON & {
    return this->json_;
  }

  [[nodiscard]] auto pointer() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->pointer_;
  }

  [[nodiscard]] auto keyword() const noexcept -> std::string_view {
    return this->keyword_;
  }

private:
  sourcemeta::core::JSON json_;
  sourcemeta::core::Pointer pointer_;
  std::string keyword_;
  const char *message_;
};

/// @ingroup ir
/// An error that represents an unsupported keyword value during IR compilation
class SOURCEMETA_CODEGEN_IR_EXPORT UnsupportedKeywordValueError
    : public std::exception {
public:
  UnsupportedKeywordValueError(sourcemeta::core::JSON json,
                               sourcemeta::core::Pointer pointer,
                               std::string keyword, const char *message)
      : json_{std::move(json)}, pointer_{std::move(pointer)},
        keyword_{std::move(keyword)}, message_{message} {}
  UnsupportedKeywordValueError(sourcemeta::core::JSON json,
                               const sourcemeta::core::WeakPointer &pointer,
                               std::string keyword, const char *message)
      : UnsupportedKeywordValueError{std::move(json),
                                     sourcemeta::core::to_pointer(pointer),
                                     std::move(keyword), message} {}
  UnsupportedKeywordValueError(sourcemeta::core::JSON json,
                               sourcemeta::core::Pointer pointer,
                               std::string keyword,
                               std::string message) = delete;
  UnsupportedKeywordValueError(sourcemeta::core::JSON json,
                               sourcemeta::core::Pointer pointer,
                               std::string keyword,
                               std::string &&message) = delete;
  UnsupportedKeywordValueError(sourcemeta::core::JSON json,
                               sourcemeta::core::Pointer pointer,
                               std::string keyword,
                               std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto json() const noexcept -> const sourcemeta::core::JSON & {
    return this->json_;
  }

  [[nodiscard]] auto pointer() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->pointer_;
  }

  [[nodiscard]] auto keyword() const noexcept -> std::string_view {
    return this->keyword_;
  }

private:
  sourcemeta::core::JSON json_;
  sourcemeta::core::Pointer pointer_;
  std::string keyword_;
  const char *message_;
};

/// @ingroup ir
/// An error that represents an unexpected schema during IR compilation
class SOURCEMETA_CODEGEN_IR_EXPORT UnexpectedSchemaError
    : public std::exception {
public:
  UnexpectedSchemaError(sourcemeta::core::JSON json,
                        sourcemeta::core::Pointer pointer, const char *message)
      : json_{std::move(json)}, pointer_{std::move(pointer)},
        message_{message} {}
  UnexpectedSchemaError(sourcemeta::core::JSON json,
                        const sourcemeta::core::WeakPointer &pointer,
                        const char *message)
      : UnexpectedSchemaError{std::move(json),
                              sourcemeta::core::to_pointer(pointer), message} {}
  UnexpectedSchemaError(sourcemeta::core::JSON json,
                        sourcemeta::core::Pointer pointer,
                        std::string message) = delete;
  UnexpectedSchemaError(sourcemeta::core::JSON json,
                        sourcemeta::core::Pointer pointer,
                        std::string &&message) = delete;
  UnexpectedSchemaError(sourcemeta::core::JSON json,
                        sourcemeta::core::Pointer pointer,
                        std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto json() const noexcept -> const sourcemeta::core::JSON & {
    return this->json_;
  }

  [[nodiscard]] auto pointer() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->pointer_;
  }

private:
  sourcemeta::core::JSON json_;
  sourcemeta::core::Pointer pointer_;
  const char *message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::codegen

#endif
