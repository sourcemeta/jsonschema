#include <sourcemeta/core/json.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonl_iterator.h>

#include "grammar.h"

#include <cassert> // assert
#include <istream> // std::basic_istream
#include <string>  // std::basic_string

namespace sourcemeta::core {

struct ConstJSONLIterator::Internal {
  sourcemeta::core::JSON current;
};

/*
 * Parsing
 */

auto ConstJSONLIterator::parse_next() -> JSON {
  // Each line in a JSONL stream is a complete JSON value.
  // See https://jsonlines.org
  std::basic_string<JSON::Char, JSON::CharTraits> row;
  while ((this->data_ != nullptr) && std::getline(*this->data_, row)) {
    this->line_ += 1;
    this->column_ = 0;

    // Strip trailing carriage return for \r\n line endings
    if (!row.empty() &&
        row.back() ==
            internal::TOKEN_JSONL_WHITESPACE_CARRIAGE_RETURN<JSON::Char>) {
      row.pop_back();
    }

    // Skip whitespace-only lines
    bool has_content{false};
    for (const auto character : row) {
      if (character != internal::TOKEN_JSONL_WHITESPACE_SPACE<JSON::Char> &&
          character !=
              internal::TOKEN_JSONL_WHITESPACE_TABULATION<JSON::Char> &&
          character !=
              internal::TOKEN_JSONL_WHITESPACE_CARRIAGE_RETURN<JSON::Char>) {
        has_content = true;
        break;
      }
    }

    if (!has_content) {
      continue;
    }

    auto result{parse_json(row, this->line_, this->column_)};

    // Verify that the remainder of the line is only whitespace
    for (auto index{static_cast<std::size_t>(this->column_)};
         index < row.size(); ++index) {
      if (row[index] != internal::TOKEN_JSONL_WHITESPACE_SPACE<JSON::Char> &&
          row[index] !=
              internal::TOKEN_JSONL_WHITESPACE_TABULATION<JSON::Char> &&
          row[index] !=
              internal::TOKEN_JSONL_WHITESPACE_CARRIAGE_RETURN<JSON::Char>) {
        this->column_ = static_cast<std::uint64_t>(index) + 1;
        throw JSONParseError(this->line_, this->column_);
      }
    }

    return result;
  }

  this->data_ = nullptr;
  return JSON{nullptr};
}

auto ConstJSONLIterator::operator++() -> ConstJSONLIterator & {
  assert(this->data_);
  this->internal_->current = this->parse_next();
  return *this;
}

/*
 * Miscellaneous
 */

ConstJSONLIterator::ConstJSONLIterator(
    std::basic_istream<JSON::Char, JSON::CharTraits> *stream)
    : data_{stream}, internal_{new Internal({this->parse_next()})} {}

ConstJSONLIterator::~ConstJSONLIterator() = default;

auto operator==(const ConstJSONLIterator &left, const ConstJSONLIterator &right)
    -> bool {
  return ((left.data_ == nullptr) && (right.data_ == nullptr)) ||
         ((left.data_ != nullptr) && (right.data_ != nullptr) &&
          left.internal_->current == right.internal_->current);
};

auto ConstJSONLIterator::operator*() const -> ConstJSONLIterator::reference {
  assert(this->data_);
  return this->internal_->current;
}

auto ConstJSONLIterator::operator->() const -> ConstJSONLIterator::pointer {
  assert(this->data_);
  return &(this->internal_->current);
}

} // namespace sourcemeta::core
