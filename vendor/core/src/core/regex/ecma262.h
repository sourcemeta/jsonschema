#ifndef SOURCEMETA_CORE_REGEX_ECMA262_H_
#define SOURCEMETA_CORE_REGEX_ECMA262_H_

#include <sourcemeta/core/unicode.h>

#include "ecma262_properties.h"

#include <cstddef>       // std::size_t
#include <optional>      // std::optional
#include <string>        // std::string, std::u32string
#include <string_view>   // std::string_view, std::u32string_view
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

namespace sourcemeta::core {

namespace {

// Every code point is a possible member of a pattern, including the null one,
// so running past the end is reported with a value outside the Unicode range
constexpr char32_t ECMA262_END{0xFFFFFFFF};

constexpr char32_t ECMA262_LAST_CODE_POINT{0x10FFFF};
constexpr char32_t ECMA262_LEAD_SURROGATE_FIRST{0xD800};
constexpr char32_t ECMA262_LEAD_SURROGATE_LAST{0xDBFF};
constexpr char32_t ECMA262_TRAIL_SURROGATE_FIRST{0xDC00};
constexpr char32_t ECMA262_TRAIL_SURROGATE_LAST{0xDFFF};
constexpr char32_t ECMA262_ZERO_WIDTH_NON_JOINER{0x200C};
constexpr char32_t ECMA262_ZERO_WIDTH_JOINER{0x200D};

constexpr std::u32string_view ECMA262_SYNTAX_CHARACTERS{U"^$\\.*+?()[]{}|"};
constexpr std::u32string_view ECMA262_CLASS_SHORTHANDS{U"dDsSwW"};
constexpr std::u32string_view ECMA262_MODIFIERS{U"ims"};

// The characters that set notation reserves for its own operators, and the
// ones it only accepts escaped so that future syntax stays available
constexpr std::u32string_view ECMA262_SET_SYNTAX_CHARACTERS{U"()[]{}/-\\|"};
constexpr std::u32string_view ECMA262_SET_RESERVED_PUNCTUATORS{
    U"&-!#%,:;<=>@`~"};
constexpr std::u32string_view ECMA262_SET_DOUBLED_PUNCTUATORS{
    U"&!#$%*+,.:;<=>?@^`~"};

// The outcome of reading one member of a character class, where a shorthand
// or a property stands for a set rather than for a single code point
struct Ecma262ClassAtom {
  bool is_class;
  char32_t value;
};

// The outcome of reading one operand of a set expression, which additionally
// tracks whether it can stand for something other than a single code point
struct Ecma262SetOperand {
  bool is_character;
  char32_t value;
  bool may_contain_strings;
  bool is_range;
};

inline auto is_set_syntax_character(const char32_t codepoint) -> bool {
  return ECMA262_SET_SYNTAX_CHARACTERS.find(codepoint) !=
         std::u32string_view::npos;
}

inline auto is_set_reserved_punctuator(const char32_t codepoint) -> bool {
  return ECMA262_SET_RESERVED_PUNCTUATORS.find(codepoint) !=
         std::u32string_view::npos;
}

inline auto is_ascii_letter(const char32_t codepoint) -> bool {
  return (codepoint >= U'a' && codepoint <= U'z') ||
         (codepoint >= U'A' && codepoint <= U'Z');
}

inline auto is_decimal_digit(const char32_t codepoint) -> bool {
  return codepoint >= U'0' && codepoint <= U'9';
}

inline auto is_hex_code_point(const char32_t codepoint) -> bool {
  return is_decimal_digit(codepoint) ||
         (codepoint >= U'a' && codepoint <= U'f') ||
         (codepoint >= U'A' && codepoint <= U'F');
}

inline auto hex_code_point_value(const char32_t codepoint) -> char32_t {
  if (is_decimal_digit(codepoint)) {
    return codepoint - U'0';
  }

  return (codepoint | 0x20) - U'a' + 10;
}

inline auto is_identifier_start(const char32_t codepoint) -> bool {
  return is_id_start(codepoint) || codepoint == U'$' || codepoint == U'_';
}

inline auto is_identifier_part(const char32_t codepoint) -> bool {
  return is_id_continue(codepoint) || codepoint == U'$' ||
         codepoint == ECMA262_ZERO_WIDTH_NON_JOINER ||
         codepoint == ECMA262_ZERO_WIDTH_JOINER;
}

inline auto significant_digits(const std::u32string_view digits)
    -> std::u32string_view {
  const auto first{digits.find_first_not_of(U'0')};
  return first == std::u32string_view::npos ? std::u32string_view{}
                                            : digits.substr(first);
}

// The digits of a quantifier bound are unbounded in the grammar, so the two
// bounds are ordered as decimal strings rather than as machine integers
inline auto decimal_greater(const std::u32string_view left,
                            const std::u32string_view right) -> bool {
  const auto left_value{significant_digits(left)};
  const auto right_value{significant_digits(right)};
  return left_value.size() == right_value.size()
             ? left_value > right_value
             : left_value.size() > right_value.size();
}

// Groups and classes both nest, and reading them is recursive, so a pattern
// that nests past this many levels is turned down rather than allowed to run
// the stack out on input that may not be trusted. No real pattern comes close,
// and engines impose a bound of their own for the same reason
constexpr std::size_t ECMA262_MAXIMUM_DEPTH{256};

// A recursive descent reader of the pattern grammar of ECMA-262, taken with
// the Unicode parameter and without the one for set notation. Constraints
// that look ahead of the point where they are written, such as a reference to
// a group that appears later, are collected and settled once the whole
// pattern has been read
struct Ecma262Reader {
  std::u32string_view input{};
  bool unicode_sets{false};
  std::size_t depth{0};
  std::size_t position{0};
  std::size_t capture_count{0};
  // The names that the alternative being read has declared, at every level of
  // alternation, as a name may only repeat across alternatives
  std::vector<std::unordered_set<std::u32string>> declared;
  std::unordered_set<std::u32string> names;
  std::vector<std::u32string> references;
  std::u32string largest_reference;

  [[nodiscard]] auto exhausted() const -> bool {
    return this->position >= this->input.size();
  }

  [[nodiscard]] auto peek(const std::size_t offset = 0) const -> char32_t {
    return this->position + offset < this->input.size()
               ? this->input[this->position + offset]
               : ECMA262_END;
  }

  auto consume(const char32_t codepoint) -> bool {
    if (this->peek() == codepoint) {
      this->position += 1;
      return true;
    }

    return false;
  }

  auto read_digits() -> std::u32string_view {
    const auto start{this->position};
    while (is_decimal_digit(this->peek())) {
      this->position += 1;
    }

    return this->input.substr(start, this->position - start);
  }

  auto read_pattern() -> bool {
    if (!this->read_disjunction() || !this->exhausted()) {
      return false;
    }

    for (const auto &reference : this->references) {
      if (!this->names.contains(reference)) {
        return false;
      }
    }

    if (!this->largest_reference.empty() &&
        decimal_greater(this->largest_reference,
                        to_decimal_string(this->capture_count))) {
      return false;
    }

    return true;
  }

  static auto to_decimal_string(const std::size_t value) -> std::u32string {
    if (value == 0) {
      return U"0";
    }

    std::u32string result;
    for (auto remaining = value; remaining > 0; remaining /= 10) {
      result.insert(result.begin(),
                    static_cast<char32_t>(U'0' + (remaining % 10)));
    }

    return result;
  }

  auto read_disjunction() -> bool {
    if (this->depth >= ECMA262_MAXIMUM_DEPTH) {
      return false;
    }

    this->depth += 1;
    const auto result{this->read_disjunction_contents()};
    this->depth -= 1;
    return result;
  }

  auto read_disjunction_contents() -> bool {
    this->declared.emplace_back();
    std::unordered_set<std::u32string> gathered;
    if (!this->read_alternative()) {
      return false;
    }

    while (this->consume(U'|')) {
      gathered.merge(this->declared.back());
      this->declared.back().clear();
      if (!this->read_alternative()) {
        return false;
      }
    }

    gathered.merge(this->declared.back());
    this->declared.pop_back();

    // What every alternative declared belongs to the alternative that encloses
    // them, where the same name may not already stand
    if (this->declared.empty()) {
      return true;
    }

    for (const auto &name : gathered) {
      if (!this->declared.back().insert(name).second) {
        return false;
      }
    }

    return true;
  }

  auto read_alternative() -> bool {
    while (!this->exhausted() && this->peek() != U'|' && this->peek() != U')') {
      if (!this->read_term()) {
        return false;
      }
    }

    return true;
  }

  auto read_term() -> bool {
    const auto current{this->peek()};
    // An assertion is its own kind of term and takes no quantifier, so a
    // quantifier written after one has nothing to repeat
    if (current == U'^' || current == U'$') {
      this->position += 1;
      return true;
    }

    if (current == U'\\' && (this->peek(1) == U'b' || this->peek(1) == U'B')) {
      this->position += 2;
      return true;
    }

    if (current == U'(' && this->peek(1) == U'?') {
      const auto third{this->peek(2)};
      if (third == U'=' || third == U'!') {
        this->position += 3;
        return this->read_disjunction() && this->consume(U')');
      }

      if (third == U'<' && (this->peek(3) == U'=' || this->peek(3) == U'!')) {
        this->position += 4;
        return this->read_disjunction() && this->consume(U')');
      }
    }

    return this->read_atom() && this->read_quantifier();
  }

  auto read_quantifier() -> bool {
    const auto current{this->peek()};
    if (current == U'*' || current == U'+' || current == U'?') {
      this->position += 1;
    } else if (current == U'{') {
      if (!this->read_braced_quantifier()) {
        return false;
      }
    } else {
      return true;
    }

    this->consume(U'?');
    return true;
  }

  auto read_braced_quantifier() -> bool {
    this->position += 1;
    const auto minimum{this->read_digits()};
    if (minimum.empty()) {
      return false;
    }

    if (this->consume(U'}')) {
      return true;
    }

    if (!this->consume(U',')) {
      return false;
    }

    if (this->consume(U'}')) {
      return true;
    }

    const auto maximum{this->read_digits()};
    return !maximum.empty() && this->consume(U'}') &&
           !decimal_greater(minimum, maximum);
  }

  auto read_atom() -> bool {
    const auto current{this->peek()};
    if (current == U'.') {
      this->position += 1;
      return true;
    }

    if (current == U'\\') {
      this->position += 1;
      return this->read_atom_escape();
    }

    if (current == U'[') {
      return this->read_character_class();
    }

    if (current == U'(') {
      return this->read_group();
    }

    if (current == ECMA262_END ||
        ECMA262_SYNTAX_CHARACTERS.find(current) != std::u32string_view::npos) {
      return false;
    }

    this->position += 1;
    return true;
  }

  auto read_group() -> bool {
    this->position += 1;
    if (this->peek() != U'?') {
      this->capture_count += 1;
      return this->read_disjunction() && this->consume(U')');
    }

    if (this->peek(1) == U'<') {
      this->position += 1;
      auto name{this->read_group_name()};
      if (!name.has_value()) {
        return false;
      }

      this->capture_count += 1;
      if (!this->declared.back().insert(name.value()).second) {
        return false;
      }

      this->names.insert(std::move(name.value()));
      return this->read_disjunction() && this->consume(U')');
    }

    return this->read_modifiers() && this->read_disjunction() &&
           this->consume(U')');
  }

  // A group that carries no name carries flags instead, and the plain
  // non-capturing form is the one that turns none of them on or off
  auto read_modifiers() -> bool {
    this->position += 1;
    const auto added{this->read_modifier_run()};
    std::u32string removed;
    const bool subtracts{this->consume(U'-')};
    if (subtracts) {
      removed = this->read_modifier_run();
    }

    if (!this->consume(U':')) {
      return false;
    }

    if (subtracts && added.empty() && removed.empty()) {
      return false;
    }

    return !has_repeated_modifier(added) && !has_repeated_modifier(removed) &&
           !shares_modifier(added, removed);
  }

  auto read_modifier_run() -> std::u32string {
    std::u32string result;
    while (ECMA262_MODIFIERS.find(this->peek()) != std::u32string_view::npos) {
      result += this->peek();
      this->position += 1;
    }

    return result;
  }

  static auto has_repeated_modifier(const std::u32string_view run) -> bool {
    for (std::size_t left = 0; left < run.size(); ++left) {
      for (auto right = left + 1; right < run.size(); ++right) {
        if (run[left] == run[right]) {
          return true;
        }
      }
    }

    return false;
  }

  static auto shares_modifier(const std::u32string_view left,
                              const std::u32string_view right) -> bool {
    for (const auto codepoint : left) {
      if (right.find(codepoint) != std::u32string_view::npos) {
        return true;
      }
    }

    return false;
  }

  auto read_group_name() -> std::optional<std::u32string> {
    if (!this->consume(U'<')) {
      return std::nullopt;
    }

    std::u32string name;
    const auto first{this->read_identifier_code_point()};
    if (!first.has_value() || !is_identifier_start(first.value())) {
      return std::nullopt;
    }

    name += first.value();
    while (this->peek() != U'>') {
      const auto next{this->read_identifier_code_point()};
      if (!next.has_value() || !is_identifier_part(next.value())) {
        return std::nullopt;
      }

      name += next.value();
    }

    this->position += 1;
    return name;
  }

  auto read_identifier_code_point() -> std::optional<char32_t> {
    const auto current{this->peek()};
    if (current == ECMA262_END) {
      return std::nullopt;
    }

    if (current == U'\\') {
      if (this->peek(1) != U'u') {
        return std::nullopt;
      }

      this->position += 1;
      return this->read_unicode_escape();
    }

    this->position += 1;
    return current;
  }

  auto read_atom_escape() -> bool {
    const auto current{this->peek()};
    if (current >= U'1' && current <= U'9') {
      const auto digits{this->read_digits()};
      if (decimal_greater(digits, this->largest_reference)) {
        this->largest_reference = digits;
      }

      return true;
    }

    if (current == U'k') {
      this->position += 1;
      auto name{this->read_group_name()};
      if (!name.has_value()) {
        return false;
      }

      this->references.push_back(std::move(name.value()));
      return true;
    }

    if (ECMA262_CLASS_SHORTHANDS.find(current) != std::u32string_view::npos) {
      this->position += 1;
      return true;
    }

    if (current == U'p' || current == U'P') {
      return this->read_unicode_property(current == U'P').has_value();
    }

    return this->read_character_escape().has_value();
  }

  // Reads a property expression and reports whether it may stand for
  // something other than a single code point
  auto read_unicode_property(const bool negated) -> std::optional<bool> {
    this->position += 1;
    if (!this->consume(U'{')) {
      return std::nullopt;
    }

    const auto first{this->read_property_token()};
    if (this->consume(U'=')) {
      const auto value{this->read_property_token()};
      if (first.empty() || value.empty() || !this->consume(U'}') ||
          !is_listed_property(ECMA262_NON_BINARY_PROPERTIES, first)) {
        return std::nullopt;
      }

      const bool known{
          (first == "General_Category" || first == "gc")
              ? is_listed_property(ECMA262_GENERAL_CATEGORY_VALUES, value)
              : is_listed_property(ECMA262_SCRIPT_VALUES, value)};
      return known ? std::optional<bool>{false} : std::nullopt;
    }

    if (first.empty() || !this->consume(U'}')) {
      return std::nullopt;
    }

    // A property that stands for a set of strings only resolves under set
    // notation, and never where the property is negated
    if (is_listed_property(ECMA262_BINARY_PROPERTIES_OF_STRINGS, first)) {
      return (this->unicode_sets && !negated) ? std::optional<bool>{true}
                                              : std::nullopt;
    }

    if (is_listed_property(ECMA262_GENERAL_CATEGORY_VALUES, first) ||
        is_listed_property(ECMA262_BINARY_PROPERTIES, first)) {
      return false;
    }

    return std::nullopt;
  }

  // Both halves of a property expression are spelled with ASCII alone, so the
  // token is gathered as a narrow string ready to look up
  auto read_property_token() -> std::string {
    std::string result;
    while (is_ascii_letter(this->peek()) || this->peek() == U'_' ||
           is_decimal_digit(this->peek())) {
      result += static_cast<char>(this->peek());
      this->position += 1;
    }

    return result;
  }

  auto read_character_escape() -> std::optional<char32_t> {
    const auto current{this->peek()};
    switch (current) {
      case U'f':
        this->position += 1;
        return 0x000C;
      case U'n':
        this->position += 1;
        return 0x000A;
      case U'r':
        this->position += 1;
        return 0x000D;
      case U't':
        this->position += 1;
        return 0x0009;
      case U'v':
        this->position += 1;
        return 0x000B;
      default:
        break;
    }

    if (current == U'c') {
      const auto letter{this->peek(1)};
      if (!is_ascii_letter(letter)) {
        return std::nullopt;
      }

      this->position += 2;
      return letter % 32;
    }

    if (current == U'0') {
      if (is_decimal_digit(this->peek(1))) {
        return std::nullopt;
      }

      this->position += 1;
      return 0x0000;
    }

    if (current == U'x') {
      if (!is_hex_code_point(this->peek(1)) ||
          !is_hex_code_point(this->peek(2))) {
        return std::nullopt;
      }

      const auto value{(hex_code_point_value(this->peek(1)) << 4U) |
                       hex_code_point_value(this->peek(2))};
      this->position += 3;
      return value;
    }

    if (current == U'u') {
      return this->read_unicode_escape();
    }

    if (current == U'/' ||
        ECMA262_SYNTAX_CHARACTERS.find(current) != std::u32string_view::npos) {
      this->position += 1;
      return current;
    }

    return std::nullopt;
  }

  auto read_unicode_escape() -> std::optional<char32_t> {
    this->position += 1;
    if (this->consume(U'{')) {
      if (!is_hex_code_point(this->peek())) {
        return std::nullopt;
      }

      char32_t value{0};
      while (is_hex_code_point(this->peek())) {
        value = (value << 4U) | hex_code_point_value(this->peek());
        this->position += 1;
        if (value > ECMA262_LAST_CODE_POINT) {
          return std::nullopt;
        }
      }

      return this->consume(U'}') ? std::optional<char32_t>{value}
                                 : std::nullopt;
    }

    const auto leading{this->read_four_hex_digits()};
    if (!leading.has_value()) {
      return std::nullopt;
    }

    // A pair of escapes standing for a surrogate pair denotes the single code
    // point they combine into, rather than either half on its own
    if (leading.value() >= ECMA262_LEAD_SURROGATE_FIRST &&
        leading.value() <= ECMA262_LEAD_SURROGATE_LAST &&
        this->peek() == U'\\' && this->peek(1) == U'u') {
      const auto rewind{this->position};
      this->position += 2;
      const auto trailing{this->read_four_hex_digits()};
      if (trailing.has_value() &&
          trailing.value() >= ECMA262_TRAIL_SURROGATE_FIRST &&
          trailing.value() <= ECMA262_TRAIL_SURROGATE_LAST) {
        return 0x10000 +
               ((leading.value() - ECMA262_LEAD_SURROGATE_FIRST) << 10U) +
               (trailing.value() - ECMA262_TRAIL_SURROGATE_FIRST);
      }

      this->position = rewind;
    }

    return leading;
  }

  auto read_four_hex_digits() -> std::optional<char32_t> {
    for (std::size_t offset = 0; offset < 4; ++offset) {
      if (!is_hex_code_point(this->peek(offset))) {
        return std::nullopt;
      }
    }

    char32_t value{0};
    for (std::size_t offset = 0; offset < 4; ++offset) {
      value = (value << 4U) | hex_code_point_value(this->peek(offset));
    }

    this->position += 4;
    return value;
  }

  auto read_character_class() -> bool {
    this->position += 1;
    const bool negated{this->consume(U'^')};
    if (this->unicode_sets) {
      const auto strings{this->read_set_expression()};
      // A negated class can never stand for a set of strings, as there is no
      // sensible complement of one
      return strings.has_value() && !(negated && strings.value()) &&
             this->consume(U']');
    }

    while (this->peek() != U']') {
      const auto first{this->read_class_atom()};
      if (!first.has_value()) {
        return false;
      }

      if (this->peek() != U'-' || this->peek(1) == U']' ||
          this->peek(1) == ECMA262_END) {
        continue;
      }

      this->position += 1;
      const auto second{this->read_class_atom()};
      if (!second.has_value() || first->is_class || second->is_class ||
          first->value > second->value) {
        return false;
      }
    }

    this->position += 1;
    return true;
  }

  // Reads the contents of a class written with set notation, reporting
  // whether the result may stand for something other than a single code point
  auto read_set_expression() -> std::optional<bool> {
    if (this->depth >= ECMA262_MAXIMUM_DEPTH) {
      return std::nullopt;
    }

    this->depth += 1;
    const auto result{this->read_set_expression_contents()};
    this->depth -= 1;
    return result;
  }

  auto read_set_expression_contents() -> std::optional<bool> {
    if (this->peek() == U']') {
      return false;
    }

    auto first{this->read_set_range_or_operand()};
    if (!first.has_value()) {
      return std::nullopt;
    }

    const bool operates{(this->peek() == U'&' && this->peek(1) == U'&') ||
                        (this->peek() == U'-' && this->peek(1) == U'-')};
    if (first->is_range && operates) {
      return std::nullopt;
    }

    if (this->peek() == U'&' && this->peek(1) == U'&') {
      auto strings{first->may_contain_strings};
      while (this->peek() == U'&' && this->peek(1) == U'&') {
        this->position += 2;
        if (this->peek() == U'&') {
          return std::nullopt;
        }

        const auto operand{this->read_set_operand()};
        if (!operand.has_value()) {
          return std::nullopt;
        }

        strings = strings && operand->may_contain_strings;
      }

      return this->peek() == U']' ? std::optional<bool>{strings} : std::nullopt;
    }

    if (this->peek() == U'-' && this->peek(1) == U'-') {
      while (this->peek() == U'-' && this->peek(1) == U'-') {
        this->position += 2;
        if (!this->read_set_operand().has_value()) {
          return std::nullopt;
        }
      }

      // Taking members away can never introduce one, so only what the
      // expression started from decides this
      return this->peek() == U']'
                 ? std::optional<bool>{first->may_contain_strings}
                 : std::nullopt;
    }

    auto strings{first->may_contain_strings};
    while (this->peek() != U']') {
      const auto next{this->read_set_range_or_operand()};
      if (!next.has_value()) {
        return std::nullopt;
      }

      strings = strings || next->may_contain_strings;
    }

    return strings;
  }

  auto read_set_range_or_operand() -> std::optional<Ecma262SetOperand> {
    const auto first{this->read_set_operand()};
    if (!first.has_value() || !first->is_character || this->peek() != U'-' ||
        this->peek(1) == U'-' || this->peek(1) == U']' ||
        this->peek(1) == ECMA262_END) {
      return first;
    }

    this->position += 1;
    const auto second{this->read_set_operand()};
    if (!second.has_value() || !second->is_character ||
        first->value > second->value) {
      return std::nullopt;
    }

    return Ecma262SetOperand{.is_character = false,
                             .value = 0,
                             .may_contain_strings = false,
                             .is_range = true};
  }

  auto read_set_operand() -> std::optional<Ecma262SetOperand> {
    if (this->peek() == U'[') {
      this->position += 1;
      const bool negated{this->consume(U'^')};
      const auto strings{this->read_set_expression()};
      if (!strings.has_value() || (negated && strings.value()) ||
          !this->consume(U']')) {
        return std::nullopt;
      }

      return Ecma262SetOperand{.is_character = false,
                               .value = 0,
                               .may_contain_strings = strings.value(),
                               .is_range = false};
    }

    if (this->peek() == U'\\') {
      const auto next{this->peek(1)};
      if (next == U'q') {
        this->position += 2;
        return this->read_set_string_disjunction();
      }

      if (ECMA262_CLASS_SHORTHANDS.find(next) != std::u32string_view::npos) {
        this->position += 2;
        return Ecma262SetOperand{.is_character = false,
                                 .value = 0,
                                 .may_contain_strings = false,
                                 .is_range = false};
      }

      if (next == U'p' || next == U'P') {
        this->position += 1;
        const auto strings{this->read_unicode_property(next == U'P')};
        if (!strings.has_value()) {
          return std::nullopt;
        }

        return Ecma262SetOperand{.is_character = false,
                                 .value = 0,
                                 .may_contain_strings = strings.value(),
                                 .is_range = false};
      }
    }

    const auto character{this->read_set_character()};
    if (!character.has_value()) {
      return std::nullopt;
    }

    return Ecma262SetOperand{.is_character = true,
                             .value = character.value(),
                             .may_contain_strings = false,
                             .is_range = false};
  }

  auto read_set_string_disjunction() -> std::optional<Ecma262SetOperand> {
    if (!this->consume(U'{')) {
      return std::nullopt;
    }

    bool strings{false};
    while (true) {
      std::size_t length{0};
      while (this->peek() != U'}' && this->peek() != U'|') {
        if (!this->read_set_character().has_value()) {
          return std::nullopt;
        }

        length += 1;
      }

      // An alternative of any length other than one stands for a string
      strings = strings || length != 1;
      if (!this->consume(U'|')) {
        break;
      }
    }

    if (!this->consume(U'}')) {
      return std::nullopt;
    }

    return Ecma262SetOperand{.is_character = false,
                             .value = 0,
                             .may_contain_strings = strings,
                             .is_range = false};
  }

  auto read_set_character() -> std::optional<char32_t> {
    const auto current{this->peek()};
    if (current == ECMA262_END) {
      return std::nullopt;
    }

    if (current == U'\\') {
      this->position += 1;
      if (this->peek() == U'b') {
        this->position += 1;
        return 0x0008;
      }

      if (is_set_reserved_punctuator(this->peek())) {
        const auto punctuator{this->peek()};
        this->position += 1;
        return punctuator;
      }

      return this->read_character_escape();
    }

    // Set notation keeps its own operators, and holds back every doubled
    // punctuator so that later revisions may give them a meaning
    if (is_set_syntax_character(current) ||
        (this->peek(1) == current &&
         ECMA262_SET_DOUBLED_PUNCTUATORS.find(current) !=
             std::u32string_view::npos)) {
      return std::nullopt;
    }

    this->position += 1;
    return current;
  }

  auto read_class_atom() -> std::optional<Ecma262ClassAtom> {
    const auto current{this->peek()};
    if (current == ECMA262_END) {
      return std::nullopt;
    }

    if (current != U'\\') {
      this->position += 1;
      return Ecma262ClassAtom{.is_class = false, .value = current};
    }

    this->position += 1;
    const auto next{this->peek()};
    if (next == U'b') {
      this->position += 1;
      return Ecma262ClassAtom{.is_class = false, .value = 0x0008};
    }

    if (next == U'-') {
      this->position += 1;
      return Ecma262ClassAtom{.is_class = false, .value = U'-'};
    }

    if (ECMA262_CLASS_SHORTHANDS.find(next) != std::u32string_view::npos) {
      this->position += 1;
      return Ecma262ClassAtom{.is_class = true, .value = 0};
    }

    if (next == U'p' || next == U'P') {
      return this->read_unicode_property(next == U'P').has_value()
                 ? std::optional<Ecma262ClassAtom>{Ecma262ClassAtom{
                       .is_class = true, .value = 0}}
                 : std::nullopt;
    }

    const auto value{this->read_character_escape()};
    return value.has_value() ? std::optional<Ecma262ClassAtom>{Ecma262ClassAtom{
                                   .is_class = false, .value = value.value()}}
                             : std::nullopt;
  }
};

// Whether the given text is a pattern of ECMA-262 read with the Unicode
// parameter, which is the reading that a JSON Schema regular expression takes
inline auto is_ecma262_pattern(const std::string_view pattern) -> bool {
  const auto decoded{utf8_to_utf32(pattern)};
  if (!decoded.has_value()) {
    return false;
  }

  Ecma262Reader unicode{};
  unicode.input = decoded.value();
  if (unicode.read_pattern()) {
    return true;
  }

  Ecma262Reader unicode_sets{};
  unicode_sets.input = decoded.value();
  unicode_sets.unicode_sets = true;
  return unicode_sets.read_pattern();
}

} // namespace

} // namespace sourcemeta::core

#endif
