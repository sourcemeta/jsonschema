#include <sourcemeta/core/json.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonl_iterator.h>

#include "grammar.h"

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t
#include <istream>     // std::basic_istream
#include <string>      // std::basic_string
#include <string_view> // std::basic_string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

struct ConstJSONLIterator::Internal {
  sourcemeta::core::JSON current;
};

namespace {

// RFC 7159 Section 2: "ws = *( %x20 / %x09 / %x0A / %x0D )"
auto is_json_whitespace(const JSON::Char character) -> bool {
  return character == internal::TOKEN_JSONL_WHITESPACE_SPACE<JSON::Char> ||
         character == internal::TOKEN_JSONL_WHITESPACE_TABULATION<JSON::Char> ||
         character == internal::TOKEN_JSONL_LINE_FEED<JSON::Char> ||
         character ==
             internal::TOKEN_JSONL_WHITESPACE_CARRIAGE_RETURN<JSON::Char>;
}

// Consume a span that may only carry JSON whitespace, advancing the position
// counters over it and reporting the first byte that is not whitespace,
// returning whether the span carried any whitespace at all
auto consume_whitespace(
    const std::basic_string_view<JSON::Char, JSON::CharTraits> span,
    std::uint64_t &line, std::uint64_t &column) -> bool {
  for (const auto character : span) {
    if (!is_json_whitespace(character)) {
      column += 1;
      throw JSONParseError(line, column);
    }

    if (character == internal::TOKEN_JSONL_LINE_FEED<JSON::Char>) {
      line += 1;
      column = 0;
    } else {
      column += 1;
    }
  }

  return !span.empty();
}

// RFC 7464 Section 2.4: "While objects, arrays, and strings are self-delimited
// in JSON texts, numbers and the values 'true', 'false', and 'null' are not"
auto is_self_delimiting(const JSON &document) -> bool {
  return document.is_object() || document.is_array() || document.is_string();
}

} // namespace

/*
 * Parsing
 */

auto ConstJSONLIterator::parse_next() -> JSON {
  switch (this->framing_) {
    case JSONLFraming::LineFeed:
      return this->parse_next_line();
    case JSONLFraming::RecordSeparator:
      return this->parse_next_record();
  }

  std::unreachable();
}

auto ConstJSONLIterator::parse_next_record() -> JSON {
  std::basic_string<JSON::Char, JSON::CharTraits> record;
  while ((this->data_ != nullptr) &&
         std::getline(*this->data_, record,
                      internal::TOKEN_JSONL_RECORD_SEPARATOR<JSON::Char>)) {
    if (this->at_sequence_start_) {
      this->at_sequence_start_ = false;
      this->line_ += 1;

      // RFC 7464 Section 2.1: "input-JSON-sequence = *(1*RS possible-JSON)",
      // so every element is introduced by a record separator and whatever
      // precedes the first one is not part of the sequence
      consume_whitespace(record, this->line_, this->column_);
      continue;
    }

    // The record separator that introduced this element, which the previous
    // read consumed
    this->column_ += 1;

    bool has_content{false};
    for (const auto character : record) {
      if (!is_json_whitespace(character)) {
        has_content = true;
        break;
      }
    }

    // RFC 7464 Section 2.1: "Multiple consecutive RS octets do not denote
    // empty sequence elements between them and can be ignored"
    if (!has_content) {
      consume_whitespace(record, this->line_, this->column_);
      continue;
    }

    const auto record_line{this->line_};
    const auto record_column{this->column_};
    auto result{parse_json(record, this->line_, this->column_)};

    // The parser reports where it stopped as a column within a line, so the
    // offset of the remainder is found by skipping the line feeds it consumed
    std::size_t offset{0};
    if (this->line_ == record_line) {
      offset = static_cast<std::size_t>(this->column_ - record_column);
    } else {
      for (auto consumed{this->line_ - record_line}; consumed > 0;
           consumed -= 1) {
        const auto position{
            record.find(internal::TOKEN_JSONL_LINE_FEED<JSON::Char>, offset)};
        assert(position != decltype(record)::npos);
        offset = position + 1;
      }

      offset += static_cast<std::size_t>(this->column_);
    }

    const auto delimited{consume_whitespace(
        std::basic_string_view<JSON::Char, JSON::CharTraits>{record}.substr(
            offset),
        this->line_, this->column_)};

    // RFC 7464 Section 2.4: "Parsers MUST drop JSON-text sequence elements
    // consisting of non-self-delimited top-level values that may have been
    // truncated (that are not delimited by whitespace)"
    if (!delimited && !is_self_delimiting(result)) {
      continue;
    }

    return result;
  }

  this->data_ = nullptr;
  return JSON{nullptr};
}

auto ConstJSONLIterator::parse_next_line() -> JSON {
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

    // NDJSON 3.2 states that "The parser MAY silently ignore empty lines"
    bool has_content{false};
    for (const auto character : row) {
      if (!is_json_whitespace(character)) {
        has_content = true;
        break;
      }
    }

    if (!has_content) {
      continue;
    }

    auto result{parse_json(row, this->line_, this->column_)};

    // Verify that the remainder of the line is only whitespace
    consume_whitespace(
        std::basic_string_view<JSON::Char, JSON::CharTraits>{row}.substr(
            static_cast<std::size_t>(this->column_)),
        this->line_, this->column_);

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
    std::basic_istream<JSON::Char, JSON::CharTraits> *stream,
    const JSONLFraming framing)
    : framing_{framing}, data_{stream},
      internal_{new Internal({this->parse_next()})} {}

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
