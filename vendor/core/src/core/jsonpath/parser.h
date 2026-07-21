#ifndef SOURCEMETA_CORE_JSONPATH_PARSER_H_
#define SOURCEMETA_CORE_JSONPATH_PARSER_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpath.h>
#include <sourcemeta/core/jsonpath_error.h>
#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/regex.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int64_t, std::uint32_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <variant>     // std::get, std::holds_alternative

namespace sourcemeta::core {

namespace {

// RFC 9535 Section 2.1: "an I-JSON number ... within the range of exact
// values" bounds every int production
constexpr std::int64_t JSONPATH_MAXIMUM_INTEGER{9007199254740991};

// RFC 9535 Section 4.1: crafted queries must not "trigger surprisingly
// high, possibly exponential, CPU usage or ... stack overflow", so the
// recursive filter expression productions are capped in nesting depth
constexpr std::size_t JSONPATH_MAXIMUM_NESTING_DEPTH{64};

class JSONPathParser {
public:
  JSONPathParser(const std::string_view input) : input_{input} {}

  auto parse() -> JSONPath::Query {
    // jsonpath-query = root-identifier segments
    if (this->at_end() || this->peek() != '$') {
      this->fail();
    }

    this->position_ += 1;
    JSONPath::Query result;
    bool singular{true};
    result.segments = this->parse_segments(singular);
    if (!this->at_end()) {
      this->fail();
    }

    return result;
  }

private:
  [[noreturn]] auto fail() const -> void {
    throw JSONPathParseError{this->position_ + 1};
  }

  [[nodiscard]] auto at_end() const -> bool {
    return this->position_ >= this->input_.size();
  }

  [[nodiscard]] auto peek() const -> char {
    assert(!this->at_end());
    return this->input_[this->position_];
  }

  // S = *( %x20 / %x09 / %x0A / %x0D )
  auto skip_whitespace() -> bool {
    bool consumed{false};
    while (!this->at_end()) {
      const char character{this->peek()};
      if (character == ' ' || character == '\t' || character == '\n' ||
          character == '\r') {
        this->position_ += 1;
        consumed = true;
      } else {
        break;
      }
    }

    return consumed;
  }

  // Copy one full UTF-8 encoded code point, rejecting malformed sequences,
  // overlong encodings, surrogates, and code points beyond U+10FFFF
  auto copy_character(JSON::String &output) -> void {
    const auto size{utf8_codepoint_length(this->input_, this->position_)};
    if (size == 0) {
      this->fail();
    }

    output.append(this->input_.substr(this->position_, size));
    this->position_ += size;
  }

  // segments = *(S segment)
  auto parse_segments(bool &singular) -> std::vector<JSONPath::Segment> {
    std::vector<JSONPath::Segment> segments;
    while (true) {
      const auto saved{this->position_};
      this->skip_whitespace();
      if (this->at_end() || (this->peek() != '.' && this->peek() != '[')) {
        this->position_ = saved;
        break;
      }

      auto segment{this->parse_segment(singular)};
      segment.kind = classify_segment(segment);
      segments.push_back(std::move(segment));
    }

    return segments;
  }

  [[nodiscard]] static auto classify_segment(const JSONPath::Segment &segment)
      -> JSONPath::SegmentKind {
    if (segment.selectors.size() != 1) {
      return JSONPath::SegmentKind::General;
    }

    switch (static_cast<JSONPath::SelectorKind>(
        segment.selectors.front().index())) {
      case JSONPath::SelectorKind::Name:
        return JSONPath::SegmentKind::SingleName;
      case JSONPath::SelectorKind::Index:
        return JSONPath::SegmentKind::SingleIndex;
      default:
        return JSONPath::SegmentKind::General;
    }
  }

  // segment = child-segment / descendant-segment
  auto parse_segment(bool &singular) -> JSONPath::Segment {
    JSONPath::Segment segment;
    segment.descendant = false;
    if (this->peek() == '[') {
      this->parse_bracketed_selection(segment, singular);
      return segment;
    }

    this->position_ += 1;
    if (!this->at_end() && this->peek() == '.') {
      // descendant-segment = ".." (bracketed-selection / wildcard-selector /
      // member-name-shorthand)
      this->position_ += 1;
      segment.descendant = true;
      singular = false;
      if (this->at_end()) {
        this->fail();
      }

      if (this->peek() == '[') {
        this->parse_bracketed_selection(segment, singular);
      } else if (this->peek() == '*') {
        this->position_ += 1;
        segment.selectors.emplace_back(JSONPath::SelectorWildcard{});
      } else {
        segment.selectors.emplace_back(this->parse_shorthand_name());
      }

      return segment;
    }

    // child-segment = bracketed-selection / ("." (wildcard-selector /
    // member-name-shorthand))
    if (this->at_end()) {
      this->fail();
    }

    if (this->peek() == '*') {
      this->position_ += 1;
      singular = false;
      segment.selectors.emplace_back(JSONPath::SelectorWildcard{});
    } else {
      segment.selectors.emplace_back(this->parse_shorthand_name());
    }

    return segment;
  }

  // bracketed-selection = "[" S selector *(S "," S selector) S "]"
  // Whitespace and multiple selectors disqualify the enclosing query from
  // the stricter singular-query grammar of RFC 9535 Section 2.3.5.1
  auto parse_bracketed_selection(JSONPath::Segment &segment, bool &singular)
      -> void {
    this->position_ += 1;
    bool internal_whitespace{this->skip_whitespace()};
    segment.selectors.push_back(this->parse_selector(singular));
    while (true) {
      internal_whitespace = this->skip_whitespace() || internal_whitespace;
      if (this->at_end()) {
        this->fail();
      }

      if (this->peek() == ',') {
        this->position_ += 1;
        this->skip_whitespace();
        segment.selectors.push_back(this->parse_selector(singular));
      } else if (this->peek() == ']') {
        this->position_ += 1;
        break;
      } else {
        this->fail();
      }
    }

    if (segment.selectors.size() > 1 || internal_whitespace) {
      singular = false;
    }
  }

  // selector = name-selector / wildcard-selector / slice-selector /
  // index-selector / filter-selector
  auto parse_selector(bool &singular) -> JSONPath::Selector {
    if (this->at_end()) {
      this->fail();
    }

    const char character{this->peek()};
    if (character == '"' || character == '\'') {
      auto name{this->parse_string_literal()};
      const auto hash{JSON::Object::hash(name)};
      return JSONPath::SelectorName{.name = std::move(name), .hash = hash};
    }

    if (character == '*') {
      this->position_ += 1;
      singular = false;
      return JSONPath::SelectorWildcard{};
    }

    if (character == '?') {
      // filter-selector = "?" S logical-expr
      this->position_ += 1;
      singular = false;
      this->skip_whitespace();
      return JSONPath::SelectorFilter{this->parse_logical_or()};
    }

    if (character == ':') {
      singular = false;
      return this->parse_slice(std::nullopt);
    }

    if (character == '-' || is_digit(character)) {
      const auto value{this->parse_integer()};
      // slice-selector = [start S] ":" S [end S] [":" [S step ]]
      const auto saved{this->position_};
      this->skip_whitespace();
      if (!this->at_end() && this->peek() == ':') {
        singular = false;
        return this->parse_slice(value);
      }

      this->position_ = saved;
      return JSONPath::SelectorIndex{value};
    }

    this->fail();
  }

  auto parse_slice(const std::optional<std::int64_t> start)
      -> JSONPath::SelectorSlice {
    JSONPath::SelectorSlice slice;
    slice.start = start;
    slice.step = 1;
    assert(this->peek() == ':');
    this->position_ += 1;
    this->skip_whitespace();
    if (!this->at_end() && (this->peek() == '-' || is_digit(this->peek()))) {
      slice.end = this->parse_integer();
      this->skip_whitespace();
    }

    if (!this->at_end() && this->peek() == ':') {
      this->position_ += 1;
      const auto saved{this->position_};
      this->skip_whitespace();
      if (!this->at_end() && (this->peek() == '-' || is_digit(this->peek()))) {
        slice.step = this->parse_integer();
      } else {
        this->position_ = saved;
      }
    }

    return slice;
  }

  // int = "0" / (["-"] DIGIT1 *DIGIT)
  auto parse_integer() -> std::int64_t {
    const auto begin{this->position_};
    if (this->peek() == '-') {
      this->position_ += 1;
    }

    if (this->at_end() || !is_digit(this->peek())) {
      this->fail();
    }

    if (this->peek() == '0') {
      this->position_ += 1;
      if (!this->at_end() && is_digit(this->peek())) {
        this->fail();
      }

      if (this->input_[begin] == '-') {
        this->fail();
      }

      return 0;
    }

    while (!this->at_end() && is_digit(this->peek())) {
      this->position_ += 1;
    }

    const auto value{
        to_int64_t(this->input_.substr(begin, this->position_ - begin))};
    if (!value.has_value() || value.value() > JSONPATH_MAXIMUM_INTEGER ||
        value.value() < -JSONPATH_MAXIMUM_INTEGER) {
      this->fail();
    }

    return value.value();
  }

  // string-literal = %x22 *double-quoted %x22 / %x27 *single-quoted %x27
  auto parse_string_literal() -> JSON::String {
    const char quote{this->peek()};
    this->position_ += 1;
    JSON::String result;
    while (true) {
      if (this->at_end()) {
        this->fail();
      }

      const char character{this->peek()};
      if (character == quote) {
        this->position_ += 1;
        return result;
      }

      if (character == '\\') {
        this->position_ += 1;
        this->parse_string_escape(result, quote);
        continue;
      }

      // unescaped starts at %x20, so raw control characters are not allowed
      if (static_cast<unsigned char>(character) < 0x20) {
        this->fail();
      }

      this->copy_character(result);
    }
  }

  // escapable = %x62 / %x66 / %x6E / %x72 / %x74 / "/" / "\" / (%x75 hexchar)
  // plus the quote of the enclosing kind
  auto parse_string_escape(JSON::String &result, const char quote) -> void {
    if (this->at_end()) {
      this->fail();
    }

    const char character{this->peek()};
    switch (character) {
      case 'b':
        result += '\x08';
        this->position_ += 1;
        return;
      case 'f':
        result += '\x0C';
        this->position_ += 1;
        return;
      case 'n':
        result += '\n';
        this->position_ += 1;
        return;
      case 'r':
        result += '\r';
        this->position_ += 1;
        return;
      case 't':
        result += '\t';
        this->position_ += 1;
        return;
      case '/':
        result += '/';
        this->position_ += 1;
        return;
      case '\\':
        result += '\\';
        this->position_ += 1;
        return;
      case 'u': {
        this->position_ += 1;
        const auto code{this->parse_hex_quad()};
        // hexchar = non-surrogate / (high-surrogate "\" %x75 low-surrogate)
        if (code >= 0xDC00 && code <= 0xDFFF) {
          this->fail();
        }

        if (code >= 0xD800 && code <= 0xDBFF) {
          if (this->at_end() || this->peek() != '\\') {
            this->fail();
          }

          this->position_ += 1;
          if (this->at_end() || this->peek() != 'u') {
            this->fail();
          }

          this->position_ += 1;
          const auto low{this->parse_hex_quad()};
          if (low < 0xDC00 || low > 0xDFFF) {
            this->fail();
          }

          const auto combined{0x10000 + ((code - 0xD800) << 10) +
                              (low - 0xDC00)};
          codepoint_to_utf8(static_cast<char32_t>(combined), result);
          return;
        }

        codepoint_to_utf8(static_cast<char32_t>(code), result);
        return;
      }
      default:
        if (character == quote) {
          result += quote;
          this->position_ += 1;
          return;
        }

        this->fail();
    }
  }

  auto parse_hex_quad() -> std::uint32_t {
    std::uint32_t result{0};
    for (std::size_t count{0}; count < 4; ++count) {
      if (this->at_end()) {
        this->fail();
      }

      const auto value{hex_digit_value(this->peek())};
      if (value < 0) {
        this->fail();
      }

      result = (result << 4) | static_cast<std::uint32_t>(value);
      this->position_ += 1;
    }

    return result;
  }

  // member-name-shorthand = name-first *name-char
  auto parse_shorthand_name() -> JSONPath::SelectorName {
    JSON::String name;
    if (this->at_end()) {
      this->fail();
    }

    const auto first{static_cast<unsigned char>(this->peek())};
    if (first == '_' || is_alpha(static_cast<char>(first))) {
      name += static_cast<char>(first);
      this->position_ += 1;
    } else if (first >= 0x80) {
      this->copy_character(name);
    } else {
      this->fail();
    }

    while (!this->at_end()) {
      const auto next{static_cast<unsigned char>(this->peek())};
      if (next == '_' || is_alpha(static_cast<char>(next)) ||
          is_digit(static_cast<char>(next))) {
        name += static_cast<char>(next);
        this->position_ += 1;
      } else if (next >= 0x80) {
        this->copy_character(name);
      } else {
        break;
      }
    }

    const auto hash{JSON::Object::hash(name)};
    return JSONPath::SelectorName{.name = std::move(name), .hash = hash};
  }

  // logical-or-expr = logical-and-expr *(S "||" S logical-and-expr)
  auto parse_logical_or() -> JSONPath::FilterExpression {
    // Every cycle in the filter grammar, through parentheses, nested filter
    // selectors, or function arguments, passes through this production or
    // the function one, so guarding both bounds the parser recursion
    this->depth_ += 1;
    if (this->depth_ > JSONPATH_MAXIMUM_NESTING_DEPTH) {
      this->fail();
    }

    std::vector<JSONPath::FilterExpression> children;
    children.push_back(this->parse_logical_and());
    while (true) {
      const auto saved{this->position_};
      this->skip_whitespace();
      if (this->position_ + 1 < this->input_.size() &&
          this->input_[this->position_] == '|' &&
          this->input_[this->position_ + 1] == '|') {
        this->position_ += 2;
        this->skip_whitespace();
        children.push_back(this->parse_logical_and());
      } else {
        this->position_ = saved;
        break;
      }
    }

    this->depth_ -= 1;
    if (children.size() == 1) {
      return std::move(children.front());
    }

    return JSONPath::FilterExpression{
        JSONPath::FilterDisjunction{std::move(children)}};
  }

  // logical-and-expr = basic-expr *(S "&&" S basic-expr)
  auto parse_logical_and() -> JSONPath::FilterExpression {
    std::vector<JSONPath::FilterExpression> children;
    children.push_back(this->parse_basic_expression());
    while (true) {
      const auto saved{this->position_};
      this->skip_whitespace();
      if (this->position_ + 1 < this->input_.size() &&
          this->input_[this->position_] == '&' &&
          this->input_[this->position_ + 1] == '&') {
        this->position_ += 2;
        this->skip_whitespace();
        children.push_back(this->parse_basic_expression());
      } else {
        this->position_ = saved;
        break;
      }
    }

    if (children.size() == 1) {
      return std::move(children.front());
    }

    return JSONPath::FilterExpression{
        JSONPath::FilterConjunction{std::move(children)}};
  }

  // basic-expr = paren-expr / comparison-expr / test-expr
  auto parse_basic_expression() -> JSONPath::FilterExpression {
    if (this->at_end()) {
      this->fail();
    }

    const char character{this->peek()};
    if (character == '!') {
      this->position_ += 1;
      this->skip_whitespace();
      if (this->at_end()) {
        this->fail();
      }

      if (this->peek() == '(') {
        std::vector<JSONPath::FilterExpression> children;
        children.push_back(this->parse_parenthesized());
        return JSONPath::FilterExpression{
            JSONPath::FilterNegation{.children = std::move(children)}};
      }

      return JSONPath::FilterExpression{this->parse_test(true)};
    }

    if (character == '(') {
      return this->parse_parenthesized();
    }

    return this->parse_comparison_or_test();
  }

  // paren-expr = [logical-not-op S] "(" S logical-expr S ")"
  auto parse_parenthesized() -> JSONPath::FilterExpression {
    assert(this->peek() == '(');
    this->position_ += 1;
    this->skip_whitespace();
    auto inner{this->parse_logical_or()};
    this->skip_whitespace();
    if (this->at_end() || this->peek() != ')') {
      this->fail();
    }

    this->position_ += 1;
    return inner;
  }

  // test-expr = [logical-not-op S] (filter-query / function-expr)
  auto parse_test(const bool negated) -> JSONPath::FilterTest {
    if (this->peek() == '@' || this->peek() == '$') {
      bool singular{true};
      auto query{this->parse_filter_query(singular)};
      return JSONPath::FilterTest{.negated = negated,
                                  .subject = std::move(query)};
    }

    if (is_alpha(this->peek()) && is_lowercase(this->peek())) {
      auto call{this->parse_function()};
      // RFC 9535 Section 2.4.3: a test expression admits functions whose
      // declared result is of logical or nodes type only
      if (call.function != JSONPath::FilterFunctionName::Match &&
          call.function != JSONPath::FilterFunctionName::Search) {
        this->fail();
      }

      return JSONPath::FilterTest{.negated = negated,
                                  .subject = std::move(call)};
    }

    this->fail();
  }

  [[nodiscard]] auto comparison_operator_ahead() const -> bool {
    if (this->at_end()) {
      return false;
    }

    const char character{this->input_[this->position_]};
    if (character == '<' || character == '>') {
      return true;
    }

    return (character == '=' || character == '!') &&
           this->position_ + 1 < this->input_.size() &&
           this->input_[this->position_ + 1] == '=';
  }

  // comparison-op = "==" / "!=" / "<=" / ">=" / "<" / ">"
  auto parse_comparison_operator() -> JSONPath::FilterComparisonOperator {
    const char character{this->peek()};
    if (character == '=' || character == '!') {
      this->position_ += 1;
      if (this->at_end() || this->peek() != '=') {
        this->fail();
      }

      this->position_ += 1;
      return character == '=' ? JSONPath::FilterComparisonOperator::Equal
                              : JSONPath::FilterComparisonOperator::NotEqual;
    }

    if (character == '<' || character == '>') {
      this->position_ += 1;
      const bool inclusive{!this->at_end() && this->peek() == '='};
      if (inclusive) {
        this->position_ += 1;
      }

      if (character == '<') {
        return inclusive ? JSONPath::FilterComparisonOperator::LessEqual
                         : JSONPath::FilterComparisonOperator::Less;
      }

      return inclusive ? JSONPath::FilterComparisonOperator::GreaterEqual
                       : JSONPath::FilterComparisonOperator::Greater;
    }

    this->fail();
  }

  auto parse_comparison_or_test() -> JSONPath::FilterExpression {
    const char character{this->peek()};
    if (character == '@' || character == '$') {
      bool singular{true};
      auto query{this->parse_filter_query(singular)};
      const auto saved{this->position_};
      this->skip_whitespace();
      if (this->comparison_operator_ahead()) {
        // RFC 9535 Section 2.3.5.1: "comparable = literal / singular-query /
        // function-expr"
        if (!singular) {
          this->fail();
        }

        const auto operation{this->parse_comparison_operator()};
        this->skip_whitespace();
        auto right{this->parse_comparable()};
        return JSONPath::FilterExpression{JSONPath::FilterComparison{
            .left = JSONPath::FilterOperand{.value = std::move(query)},
            .operation = operation,
            .right = std::move(right)}};
      }

      this->position_ = saved;
      return JSONPath::FilterExpression{
          JSONPath::FilterTest{.negated = false, .subject = std::move(query)}};
    }

    if (character == '"' || character == '\'' || character == '-' ||
        is_digit(character)) {
      auto literal{this->parse_filter_literal()};
      this->skip_whitespace();
      if (!this->comparison_operator_ahead()) {
        this->fail();
      }

      const auto operation{this->parse_comparison_operator()};
      this->skip_whitespace();
      auto right{this->parse_comparable()};
      return JSONPath::FilterExpression{JSONPath::FilterComparison{
          .left = JSONPath::FilterOperand{.value = std::move(literal)},
          .operation = operation,
          .right = std::move(right)}};
    }

    if (is_alpha(character) && is_lowercase(character)) {
      auto operand{this->parse_keyword_or_function()};
      const auto saved{this->position_};
      this->skip_whitespace();
      if (this->comparison_operator_ahead()) {
        if (std::holds_alternative<JSONPath::FilterFunctionCall>(
                operand.value) &&
            !is_value_type_function(
                std::get<JSONPath::FilterFunctionCall>(operand.value))) {
          this->fail();
        }

        const auto operation{this->parse_comparison_operator()};
        this->skip_whitespace();
        auto right{this->parse_comparable()};
        return JSONPath::FilterExpression{
            JSONPath::FilterComparison{.left = std::move(operand),
                                       .operation = operation,
                                       .right = std::move(right)}};
      }

      this->position_ = saved;
      if (!std::holds_alternative<JSONPath::FilterFunctionCall>(
              operand.value)) {
        // A literal is not a valid test expression
        this->fail();
      }

      auto call{
          std::move(std::get<JSONPath::FilterFunctionCall>(operand.value))};
      if (call.function != JSONPath::FilterFunctionName::Match &&
          call.function != JSONPath::FilterFunctionName::Search) {
        this->fail();
      }

      return JSONPath::FilterExpression{
          JSONPath::FilterTest{.negated = false, .subject = std::move(call)}};
    }

    this->fail();
  }

  static auto is_value_type_function(const JSONPath::FilterFunctionCall &call)
      -> bool {
    return call.function == JSONPath::FilterFunctionName::Length ||
           call.function == JSONPath::FilterFunctionName::Count ||
           call.function == JSONPath::FilterFunctionName::Value;
  }

  // comparable = literal / singular-query / function-expr
  auto parse_comparable() -> JSONPath::FilterOperand {
    if (this->at_end()) {
      this->fail();
    }

    const char character{this->peek()};
    if (character == '@' || character == '$') {
      bool singular{true};
      auto query{this->parse_filter_query(singular)};
      if (!singular) {
        this->fail();
      }

      return JSONPath::FilterOperand{.value = std::move(query)};
    }

    if (character == '"' || character == '\'' || character == '-' ||
        is_digit(character)) {
      return JSONPath::FilterOperand{.value = this->parse_filter_literal()};
    }

    if (is_alpha(character) && is_lowercase(character)) {
      auto operand{this->parse_keyword_or_function()};
      if (std::holds_alternative<JSONPath::FilterFunctionCall>(operand.value) &&
          !is_value_type_function(
              std::get<JSONPath::FilterFunctionCall>(operand.value))) {
        this->fail();
      }

      return operand;
    }

    this->fail();
  }

  // literal = number / string-literal / true / false / null
  auto parse_filter_literal() -> JSON {
    const char character{this->peek()};
    if (character == '"' || character == '\'') {
      return JSON{this->parse_string_literal()};
    }

    return this->parse_number();
  }

  // number = (int / "-0") [ frac ] [ exp ]
  auto parse_number() -> JSON {
    const auto begin{this->position_};
    if (this->peek() == '-') {
      this->position_ += 1;
    }

    if (this->at_end() || !is_digit(this->peek())) {
      this->fail();
    }

    if (this->peek() == '0') {
      this->position_ += 1;
      if (!this->at_end() && is_digit(this->peek())) {
        this->fail();
      }
    } else {
      while (!this->at_end() && is_digit(this->peek())) {
        this->position_ += 1;
      }
    }

    bool real{false};
    // frac = "." 1*DIGIT
    if (!this->at_end() && this->peek() == '.') {
      real = true;
      this->position_ += 1;
      if (this->at_end() || !is_digit(this->peek())) {
        this->fail();
      }

      while (!this->at_end() && is_digit(this->peek())) {
        this->position_ += 1;
      }
    }

    // exp = "e" [ "-" / "+" ] 1*DIGIT
    if (!this->at_end() && (this->peek() == 'e' || this->peek() == 'E')) {
      real = true;
      this->position_ += 1;
      if (!this->at_end() && (this->peek() == '-' || this->peek() == '+')) {
        this->position_ += 1;
      }

      if (this->at_end() || !is_digit(this->peek())) {
        this->fail();
      }

      while (!this->at_end() && is_digit(this->peek())) {
        this->position_ += 1;
      }
    }

    const auto text{this->input_.substr(begin, this->position_ - begin)};
    if (!real) {
      const auto value{to_int64_t(text)};
      if (!value.has_value() || value.value() > JSONPATH_MAXIMUM_INTEGER ||
          value.value() < -JSONPATH_MAXIMUM_INTEGER) {
        this->fail();
      }

      return JSON{value.value()};
    }

    const auto value{to_double(text)};
    if (!value.has_value()) {
      this->fail();
    }

    return JSON{value.value()};
  }

  // filter-query = rel-query / jsonpath-query
  auto parse_filter_query(bool &singular) -> JSONPath::FilterQuery {
    JSONPath::FilterQuery query;
    query.relative = this->peek() == '@';
    this->position_ += 1;
    query.segments = this->parse_segments(singular);
    query.singular = singular;
    return query;
  }

  auto parse_identifier() -> std::string_view {
    const auto begin{this->position_};
    while (!this->at_end() &&
           ((is_alpha(this->peek()) && is_lowercase(this->peek())) ||
            is_digit(this->peek()) || this->peek() == '_')) {
      this->position_ += 1;
    }

    return this->input_.substr(begin, this->position_ - begin);
  }

  auto parse_keyword_or_function() -> JSONPath::FilterOperand {
    const auto saved{this->position_};
    const auto identifier{this->parse_identifier()};
    if (!this->at_end() && this->peek() == '(') {
      this->position_ = saved;
      return JSONPath::FilterOperand{.value = this->parse_function()};
    }

    if (identifier == "true") {
      return JSONPath::FilterOperand{.value = JSON{true}};
    }

    if (identifier == "false") {
      return JSONPath::FilterOperand{.value = JSON{false}};
    }

    if (identifier == "null") {
      return JSONPath::FilterOperand{.value = JSON{nullptr}};
    }

    this->fail();
  }

  // function-expr = function-name "(" S [function-argument
  //   *(S "," S function-argument)] S ")"
  auto parse_function() -> JSONPath::FilterFunctionCall {
    this->depth_ += 1;
    if (this->depth_ > JSONPATH_MAXIMUM_NESTING_DEPTH) {
      this->fail();
    }

    const auto identifier{this->parse_identifier()};
    JSONPath::FilterFunctionCall call;
    if (identifier == "length") {
      call.function = JSONPath::FilterFunctionName::Length;
    } else if (identifier == "count") {
      call.function = JSONPath::FilterFunctionName::Count;
    } else if (identifier == "match") {
      call.function = JSONPath::FilterFunctionName::Match;
    } else if (identifier == "search") {
      call.function = JSONPath::FilterFunctionName::Search;
    } else if (identifier == "value") {
      call.function = JSONPath::FilterFunctionName::Value;
    } else {
      this->fail();
    }

    if (this->at_end() || this->peek() != '(') {
      this->fail();
    }

    this->position_ += 1;
    this->skip_whitespace();
    if (this->at_end()) {
      this->fail();
    }

    if (this->peek() != ')') {
      while (true) {
        call.arguments.push_back(this->parse_function_argument());
        this->skip_whitespace();
        if (!this->at_end() && this->peek() == ',') {
          this->position_ += 1;
          this->skip_whitespace();
        } else {
          break;
        }
      }
    }

    if (this->at_end() || this->peek() != ')') {
      this->fail();
    }

    this->position_ += 1;
    this->check_function(call);
    this->depth_ -= 1;
    return call;
  }

  auto parse_function_argument() -> JSONPath::FilterOperand {
    if (this->at_end()) {
      this->fail();
    }

    const char character{this->peek()};
    if (character == '@' || character == '$') {
      bool singular{true};
      return JSONPath::FilterOperand{.value =
                                         this->parse_filter_query(singular)};
    }

    if (character == '"' || character == '\'' || character == '-' ||
        is_digit(character)) {
      return JSONPath::FilterOperand{.value = this->parse_filter_literal()};
    }

    if (is_alpha(character) && is_lowercase(character)) {
      return this->parse_keyword_or_function();
    }

    // The grammar also admits a logical expression argument, yet none of
    // the RFC 9535 functions declares a parameter of logical type, so such
    // an argument can never be well-typed per Section 2.4.3
    this->fail();
  }

  [[nodiscard]] static auto
  is_value_type_operand(const JSONPath::FilterOperand &operand) -> bool {
    if (std::holds_alternative<JSON>(operand.value)) {
      return true;
    }

    if (std::holds_alternative<JSONPath::FilterQuery>(operand.value)) {
      return std::get<JSONPath::FilterQuery>(operand.value).singular;
    }

    return is_value_type_function(
        std::get<JSONPath::FilterFunctionCall>(operand.value));
  }

  // RFC 9535 Section 2.4.3: "the function's arguments need to be well-typed
  // against the declared parameter types"
  auto check_function(JSONPath::FilterFunctionCall &call) -> void {
    switch (call.function) {
      case JSONPath::FilterFunctionName::Length:
        if (call.arguments.size() != 1 ||
            !is_value_type_operand(call.arguments.front())) {
          this->fail();
        }

        return;
      case JSONPath::FilterFunctionName::Count:
      case JSONPath::FilterFunctionName::Value:
        if (call.arguments.size() != 1 ||
            !std::holds_alternative<JSONPath::FilterQuery>(
                call.arguments.front().value)) {
          this->fail();
        }

        return;
      case JSONPath::FilterFunctionName::Match:
      case JSONPath::FilterFunctionName::Search:
        if (call.arguments.size() != 2 ||
            !is_value_type_operand(call.arguments.front()) ||
            !is_value_type_operand(call.arguments.back())) {
          this->fail();
        }

        if (std::holds_alternative<JSON>(call.arguments.back().value) &&
            std::get<JSON>(call.arguments.back().value).is_string()) {
          call.compiled =
              to_regex(std::get<JSON>(call.arguments.back().value).to_string(),
                       call.function == JSONPath::FilterFunctionName::Match
                           ? RegexDialect::IRegexp
                           : RegexDialect::IRegexpSearch);
        }

        return;
    }
  }

  std::string_view input_;
  std::size_t position_{0};
  std::size_t depth_{0};
};

} // namespace

inline auto parse_jsonpath(const std::string_view input) -> JSONPath::Query {
  JSONPathParser parser{input};
  return parser.parse();
}

} // namespace sourcemeta::core

#endif
