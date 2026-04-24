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
  while (this->data && std::getline(*this->data, row)) {
    this->line += 1;
    this->column = 0;

    // Strip trailing carriage return for \r\n line endings
    if (!row.empty() &&
        row.back() ==
            internal::token_jsonl_whitespace_carriage_return<JSON::Char>) {
      row.pop_back();
    }

    // Skip whitespace-only lines
    bool has_content{false};
    for (const auto character : row) {
      if (character != internal::token_jsonl_whitespace_space<JSON::Char> &&
          character !=
              internal::token_jsonl_whitespace_tabulation<JSON::Char> &&
          character !=
              internal::token_jsonl_whitespace_carriage_return<JSON::Char>) {
        has_content = true;
        break;
      }
    }

    if (!has_content) {
      continue;
    }

    auto result{parse_json(row, this->line, this->column)};

    // Verify that the remainder of the line is only whitespace
    for (auto index{static_cast<std::size_t>(this->column)}; index < row.size();
         ++index) {
      if (row[index] != internal::token_jsonl_whitespace_space<JSON::Char> &&
          row[index] !=
              internal::token_jsonl_whitespace_tabulation<JSON::Char> &&
          row[index] !=
              internal::token_jsonl_whitespace_carriage_return<JSON::Char>) {
        this->column = static_cast<std::uint64_t>(index) + 1;
        throw JSONParseError(this->line, this->column);
      }
    }

    return result;
  }

  this->data = nullptr;
  return JSON{nullptr};
}

auto ConstJSONLIterator::operator++() -> ConstJSONLIterator & {
  assert(this->data);
  this->internal->current = this->parse_next();
  return *this;
}

/*
 * Miscellaneous
 */

ConstJSONLIterator::ConstJSONLIterator(
    std::basic_istream<JSON::Char, JSON::CharTraits> *stream)
    : data{stream}, internal{new Internal({this->parse_next()})} {}

ConstJSONLIterator::~ConstJSONLIterator() = default;

auto operator==(const ConstJSONLIterator &left, const ConstJSONLIterator &right)
    -> bool {
  return (!left.data && !right.data) ||
         (left.data && right.data &&
          left.internal->current == right.internal->current);
};

auto ConstJSONLIterator::operator*() const -> ConstJSONLIterator::reference {
  assert(this->data);
  return this->internal->current;
}

auto ConstJSONLIterator::operator->() const -> ConstJSONLIterator::pointer {
  assert(this->data);
  return &(this->internal->current);
}

} // namespace sourcemeta::core
