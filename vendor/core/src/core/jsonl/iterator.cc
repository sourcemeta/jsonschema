#include <sourcemeta/core/json.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonl_iterator.h>

#include "grammar.h"

#include <cassert> // assert
#include <istream> // std::basic_istream

namespace sourcemeta::core {

struct ConstJSONLIterator::Internal {
  sourcemeta::core::JSON current;
};

/*
 * Parsing
 */

auto ConstJSONLIterator::parse_next() -> JSON {
  while (this->data && !this->data->eof()) {
    switch (this->data->peek()) {
      // Whitespace
      case internal::token_jsonl_whitespace_space<JSON::Char>:
      case internal::token_jsonl_whitespace_tabulation<JSON::Char>:
      case internal::token_jsonl_whitespace_carriage_return<JSON::Char>:
        this->column += 1;
        this->data->ignore(1);
        break;
      case JSON::CharTraits::eof():
        this->data = nullptr;
        break;
      default:
        goto parse_start;
    }
  }

parse_start:
  if (this->data) {
    assert(!this->data->eof());
    return parse_json(*this->data, this->line, this->column);
  } else {
    // Just as a cheap placeholder
    return JSON{nullptr};
  }
}

auto ConstJSONLIterator::operator++() -> ConstJSONLIterator & {
  assert(this->data);

start:
  switch (this->data->get()) {
    // Separator
    case internal::token_jsonl_line_feed<JSON::Char>:
      this->line += 1;
      this->column = 0;
      goto element;

    // Whitespace
    case internal::token_jsonl_whitespace_space<JSON::Char>:
    case internal::token_jsonl_whitespace_tabulation<JSON::Char>:
    case internal::token_jsonl_whitespace_carriage_return<JSON::Char>:
      this->column += 1;
      goto start;

    case JSON::CharTraits::eof():
      goto end;
    default:
      this->column += 1;
      throw JSONParseError(this->line, this->column);
  }

element:
  switch (this->data->peek()) {
    // Whitespace
    case internal::token_jsonl_whitespace_space<JSON::Char>:
    case internal::token_jsonl_whitespace_tabulation<JSON::Char>:
    case internal::token_jsonl_whitespace_carriage_return<JSON::Char>:
      this->column += 1;
      this->data->ignore(1);
      goto element;

    case JSON::CharTraits::eof():
      goto end;
    default:
      goto next;
  }

next:
  this->internal->current = this->parse_next();
  return *this;

end:
  this->data = nullptr;
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
