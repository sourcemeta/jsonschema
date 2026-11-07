#include "internal_html.h"

#include <sourcemeta/blaze/documentation.h>

#include <sourcemeta/core/html.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>

#include <algorithm>   // std::any_of, std::all_of, std::find_if
#include <array>       // std::array
#include <cctype>      // std::toupper
#include <cmath>       // std::floor, std::fabs, std::isfinite
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint32_t, std::int64_t
#include <cstdio>      // std::snprintf
#include <deque>       // std::deque
#include <iterator>    // std::prev
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair
#include <vector>      // std::vector

// Reading the table format, and the words a page says about what it reads.

namespace {

using sourcemeta::blaze::at;
using sourcemeta::blaze::given;
using sourcemeta::blaze::is_one;
using sourcemeta::blaze::plain;
using sourcemeta::blaze::text;
using sourcemeta::core::JSON;

auto number_text(const JSON &value) -> std::string {
  if (value.is_integer()) {
    return std::to_string(value.to_integer());
  }

  if (value.is_real()) {
    const auto real{value.to_real()};
    if (std::isfinite(real) && std::floor(real) == real &&
        std::fabs(real) < 1e21) {
      std::array<char, 32> buffer{};
      std::snprintf(buffer.data(), buffer.size(), "%.0f", real);
      const std::string result{buffer.data()};
      return result == "-0" ? "0" : result;
    }
  }

  // The shortest form that reads back as the same number.
  std::ostringstream stream;
  sourcemeta::core::stringify(value, stream);
  return stream.str();
}

auto type_words(const std::string &name) -> std::string {
  static const std::map<std::string, std::string> WORDS{
      {"string", "text"},
      {"integer", "a whole number"},
      {"number", "a number"},
      {"boolean", "yes or no"},
      {"object", "a group of properties"},
      {"array", "a list"},
      {"null", "nothing"}};
  const auto match{WORDS.find(name)};
  return match == WORDS.cend() ? "undefined" : match->second;
}

// A declared format is a stronger statement than the type underneath it, so
// when one is present it is what the reader is told.
auto format_words(const JSON *format) -> std::optional<std::string> {
  static const std::map<std::string, std::string> WORDS{
      {"date-time", "a date and time"},
      {"date", "a date"},
      {"time", "a time"},
      {"uri", "a web address"},
      {"uri-reference", "a web address"},
      {"url", "a web address"},
      {"email", "an email address"},
      {"hostname", "a computer name"},
      {"ipv4", "an IP address"},
      {"ipv6", "an IP address"},
      {"uuid", "a unique id"},
      {"regex", "a search pattern"}};
  if (format == nullptr) {
    return std::nullopt;
  }

  const auto match{WORDS.find(text(format))};
  if (match == WORDS.cend()) {
    return std::nullopt;
  }

  return match->second;
}

// A value with nothing on it beyond its kind, which a short phrase says fully.
auto is_simple(const JSON *value) -> bool {
  if (value == nullptr) {
    return false;
  }

  if (!value->is_object()) {
    return true;
  }

  return std::all_of(value->as_object().cbegin(), value->as_object().cend(),
                     [](const auto &entry) {
                       return entry.first == "kind" ||
                              entry.first == "format" ||
                              entry.first == "title" ||
                              entry.first == "description";
                     });
}

// The facts a value carries beyond its kind, in a few words.
// The word for one of what a count counts.
auto singular(const std::string &unit) -> std::string {
  if (unit == "entries") {
    return "entry";
  }

  if (unit == "properties") {
    return "property";
  }

  return unit == "characters" ? "character" : unit;
}

// A count between two bounds, in words.
auto bounds_words(const JSON *bounds, const std::string &unit) -> std::string {
  const auto *minimum{at(bounds, "min")};
  const auto *maximum{at(bounds, "max")};
  if (given(minimum) && given(maximum)) {
    return plain(minimum) + " to " + plain(maximum) + " " + unit;
  }

  if (given(minimum)) {
    return "at least " + plain(minimum) + " " +
           (is_one(minimum) ? singular(unit) : unit);
  }

  return given(maximum) ? "at most " + plain(maximum) + " " +
                              (is_one(maximum) ? singular(unit) : unit)
                        : std::string{};
}

} // namespace

namespace sourcemeta::blaze {

auto at(const JSON *value, const std::string &key) -> const JSON * {
  if (value == nullptr || !value->is_object() || !value->defines(key)) {
    return nullptr;
  }

  return &value->at(key);
}

auto truthy(const JSON *value) -> bool {
  if (value == nullptr || value->is_null()) {
    return false;
  }

  if (value->is_boolean()) {
    return value->to_boolean();
  }

  if (value->is_integer()) {
    return value->to_integer() != 0;
  }

  if (value->is_real()) {
    return value->to_real() != 0 && !std::isnan(value->to_real());
  }

  if (value->is_string()) {
    return !value->to_string().empty();
  }

  if (value->is_decimal()) {
    return !(*value == JSON{static_cast<std::int64_t>(0)});
  }

  return true;
}

// Present and not null.
auto given(const JSON *value) -> bool {
  return value != nullptr && !value->is_null();
}

auto list(const JSON *value, const std::string &key)
    -> std::vector<const JSON *> {
  std::vector<const JSON *> result;
  const auto *entries{at(value, key)};
  if (entries != nullptr && entries->is_array()) {
    for (const auto &entry : entries->as_array()) {
      result.push_back(&entry);
    }
  }

  return result;
}

auto count(const JSON *value, const std::string &key) -> std::size_t {
  const auto *entries{at(value, key)};
  return entries != nullptr && entries->is_array() ? entries->array_size() : 0;
}

auto kind(const JSON *value) -> std::string {
  const auto *entry{at(value, "kind")};
  return entry != nullptr && entry->is_string() ? entry->to_string() : "";
}

auto is_one(const JSON *value) -> bool {
  return value != nullptr && value->is_number() &&
         !(*value < JSON{static_cast<std::int64_t>(1)}) &&
         !(JSON{static_cast<std::int64_t>(1)} < *value);
}

auto text(const JSON *value) -> std::string {
  if (value == nullptr) {
    return "undefined";
  }

  if (value->is_string()) {
    return value->to_string();
  }

  if (value->is_number()) {
    return number_text(*value);
  }

  if (value->is_boolean()) {
    return value->to_boolean() ? "true" : "false";
  }

  if (value->is_null()) {
    return "null";
  }

  if (value->is_array()) {
    std::string result;
    auto first{true};
    for (const auto &entry : value->as_array()) {
      if (!first) {
        result += ',';
      }

      first = false;
      if (!entry.is_null()) {
        result += text(&entry);
      }
    }

    return result;
  }

  return "[object Object]";
}

auto text(const JSON *value, const std::string &key) -> std::string {
  return text(at(value, key));
}

// Values a reader sees, never JSON.
auto plain(const JSON *value) -> std::string {
  if (value == nullptr) {
    return "undefined";
  }

  if (value->is_boolean()) {
    return value->to_boolean() ? "yes" : "no";
  }

  if (value->is_null()) {
    return "nothing";
  }

  if (value->is_array()) {
    if (value->empty()) {
      return "an empty list";
    }

    std::vector<std::string> parts;
    for (const auto &entry : value->as_array()) {
      parts.push_back(plain(&entry));
    }

    return sourcemeta::core::join(parts, ", ");
  }

  if (value->is_object()) {
    return "a group of details";
  }

  return text(value);
}

auto plain_all(const JSON *values, const std::string_view glue) -> std::string {
  std::vector<std::string> parts;
  if (values != nullptr && values->is_array()) {
    for (const auto &entry : values->as_array()) {
      parts.push_back(plain(&entry));
    }
  }

  return sourcemeta::core::join(parts, glue);
}

// Whether a value is the same as another by the strict equality of the
// reference renderer: scalars by value, anything with parts never.
auto strictly_equal(const JSON *left, const JSON *right) -> bool {
  if (left == nullptr || right == nullptr) {
    return left == right;
  }

  if (left->is_array() || left->is_object() || right->is_array() ||
      right->is_object()) {
    return false;
  }

  if (left->is_number() && right->is_number()) {
    return !(*left < *right) && !(*right < *left);
  }

  return *left == *right;
}

// An address from a schema is shown as a link only when following it is safe.
// A browser throws away tabs, newlines and control characters before it reads
// an address, so they are thrown away here too.
auto linkable(const std::string_view address) -> bool {
  const auto points{sourcemeta::core::utf8_to_utf32(address)};
  if (!points.has_value()) {
    return false;
  }

  std::string bare;
  for (const auto point : points.value()) {
    if (point > 32 && point != 127) {
      // Only the start matters, which is ASCII when it is a scheme at all.
      bare += point < 128 ? static_cast<char>(point) : '?';
    }
  }

  std::size_t position{0};
  if (!bare.empty() && sourcemeta::core::is_alpha(bare[0])) {
    position = 1;
    while (position < bare.size() &&
           (sourcemeta::core::is_alphanum(bare[position]) ||
            bare[position] == '+' || bare[position] == '.' ||
            bare[position] == '-')) {
      position += 1;
    }

    if (position < bare.size() && bare[position] == ':') {
      std::string scheme{bare.substr(0, position)};
      sourcemeta::core::to_lowercase(scheme);
      return scheme == "http" || scheme == "https" || scheme == "mailto";
    }
  }

  return !bare.starts_with("//");
}

// What a length counts, by the kind it is on.
// What a count of a kind of value counts, for one or for many.
auto count_words(const std::string &name, const bool one) -> std::string {
  if (name == "string") {
    return one ? " character" : " characters";
  }

  if (name == "array") {
    return one ? " entry" : " entries";
  }

  if (name == "object") {
    return one ? " property" : " properties";
  }

  return "";
}

auto only_keys(const JSON *value, const std::set<std::string> &allowed)
    -> bool {
  return value != nullptr && value->is_object() &&
         std::all_of(value->as_object().cbegin(), value->as_object().cend(),
                     [&allowed](const auto &entry) {
                       return allowed.contains(entry.first);
                     });
}

// What to call a shape: the author's title, or the name it is declared under.
auto shape_name(const JSON *shape) -> std::string {
  const auto *title{at(at(shape, "value"), "title")};
  if (truthy(title)) {
    return text(title);
  }

  return text(shape, "name");
}

auto Draw::name_of(const std::string &identifier) const -> std::string {
  if (identifier.empty()) {
    return "this whole document";
  }

  const auto *found{this->shape(identifier)};
  if (found != nullptr) {
    return shape_name(found);
  }

  const auto slash{identifier.rfind('/')};
  return slash == std::string::npos ? identifier : identifier.substr(slash + 1);
}

// The short phrase for a value, worked out from its facts alone.
auto label(const JSON *value, const Draw &draw) -> std::string {
  if (value == nullptr) {
    return "no rule beyond being there";
  }

  const auto name{kind(value)};
  if (name == "any") {
    return "anything";
  }

  if (name == "never") {
    return "nothing is allowed here";
  }

  if (name == "external") {
    return "follows rules kept in another file";
  }

  if (name == "ref") {
    return draw.name_of(text(value, "ref"));
  }

  if (name == "choice") {
    const auto options{list(value, "options")};
    if (!options.empty() &&
        std::all_of(options.cbegin(), options.cend(),
                    [](const auto *option) { return kind(option) == "ref"; })) {
      std::vector<std::string> names;
      names.reserve(options.size());
      for (const auto *option : options) {
        names.push_back(draw.name_of(text(option, "ref")));
      }

      return "either " + sourcemeta::core::join(names, " or ");
    }

    if (!options.empty() &&
        std::all_of(options.cbegin(), options.cend(), is_simple) &&
        !truthy(at(value, "overlap"))) {
      std::vector<std::string> phrases;
      phrases.reserve(options.size());
      for (const auto *option : options) {
        phrases.push_back(label(option, draw));
      }

      return sourcemeta::core::join(phrases, " or ");
    }

    return "one of " + std::to_string(options.size()) + " forms";
  }

  if (name == "enum") {
    return count(value, "values") == 1 ? "a fixed value" : "one of these";
  }

  if (name == "array") {
    return "a list";
  }

  if (name == "object") {
    return "a group of properties";
  }

  if (name == "boolean") {
    return "yes or no";
  }

  if (name == "null") {
    return "nothing (null)";
  }

  if (name == "string" || name == "number" || name == "integer") {
    const auto *format{at(value, "format")};
    const auto words{format_words(format)};
    if (name == "string" && truthy(format) && words.has_value()) {
      return words.value();
    }

    return type_words(name) +
           (truthy(format) && !(name == "string" && words.has_value())
                ? " (" + text(format) + ")"
                : "");
  }

  return "a value";
}

// The first example a reader can take in at a glance.
auto first_example(const JSON *value) -> const JSON * {
  for (const auto *example : list(value, "examples")) {
    if (!example->is_array() && !example->is_object()) {
      return example;
    }
  }

  return nullptr;
}

// How a pattern is presented: exactly as written, with an example if there is
// one.
auto pattern_note(const JSON *pattern, const JSON *example)
    -> std::optional<std::string> {
  if (!truthy(pattern)) {
    return std::nullopt;
  }

  auto shown{"matching " + text(pattern)};
  if (example != nullptr) {
    shown += ", for example " + plain(example);
  }

  return shown;
}

auto pattern_note(const JSON *value) -> std::optional<std::string> {
  return pattern_note(at(value, "pattern"), first_example(value));
}

auto range_word(const JSON *bounds, const std::string &edge) -> std::string {
  if (edge == "min") {
    return truthy(at(bounds, "minExclusive")) ? "above" : "from";
  }

  return truthy(at(bounds, "maxExclusive")) ? "below" : "up to";
}

auto facts_of(const JSON *value) -> std::string {
  if (value == nullptr || !value->is_object()) {
    return "";
  }

  std::vector<std::string> said;
  const auto pattern{pattern_note(value)};
  if (pattern.has_value()) {
    said.push_back(pattern.value());
  }

  if (truthy(at(value, "format"))) {
    said.push_back("written as " + text(value, "format"));
  }

  if (const auto *range{at(value, "range")}; truthy(range)) {
    if (given(at(range, "min"))) {
      said.push_back(range_word(range, "min") + " " + plain(at(range, "min")));
    }

    if (given(at(range, "max"))) {
      said.push_back(range_word(range, "max") + " " + plain(at(range, "max")));
    }
  }

  if (const auto *length{at(value, "length")}; truthy(length)) {
    said.push_back(bounds_words(length, kind(value) == "array" ? "entries"
                                                               : "characters"));
  }

  if (given(at(value, "multipleOf"))) {
    said.push_back("in steps of " + plain(at(value, "multipleOf")));
  }

  if (const auto *amount{at(value, "propertyCount")}; truthy(amount)) {
    said.push_back(bounds_words(amount, "properties"));
  }

  if (truthy(at(value, "unique"))) {
    said.emplace_back("no two entries the same");
  }

  if (truthy(at(value, "closed"))) {
    said.emplace_back("no other properties");
  }

  if (const auto *keys{at(value, "keys")};
      truthy(keys) && truthy(at(keys, "pattern"))) {
    said.push_back("names matching " + text(keys, "pattern"));
  }

  if (truthy(at(value, "nullable"))) {
    said.emplace_back("or nothing");
  }

  if (truthy(at(value, "deprecated"))) {
    said.emplace_back("no longer used");
  }

  if (truthy(at(value, "otherTypesAllowed"))) {
    said.emplace_back("other kinds pass these rules");
  }

  if (const auto more{count(value, "also")}; more > 0) {
    said.push_back("and " + (more == 1 ? std::string{"one more rule"}
                                       : std::to_string(more) + " more rules"));
  }

  if (count(value, "conditions") > 0) {
    said.emplace_back("with rules for some cases");
  }

  std::erase_if(said, [](const auto &part) { return part.empty(); });
  return sourcemeta::core::join(said, ", ");
}

// What a condition turns on, read from the value it tests.
auto condition_text(const JSON *condition) -> std::string {
  const auto *when{at(condition, "when")};
  const auto properties{kind(when) == "object" ? list(when, "properties")
                                               : std::vector<const JSON *>{}};
  if (properties.size() == 1) {
    const auto *first{properties.front()};
    const auto *value{at(first, "value")};
    if (truthy(value) && kind(value) == "enum" && count(value, "values") == 1) {
      // A test on a property's value also passes when it is not there.
      // Only text is quoted, so the number 1 does not read as the text "1".
      const auto name{text(first, "name")};
      const auto &fixed{at(value, "values")->at(0)};
      return "when " + name + " is " +
             (fixed.is_string() ? "\"" + fixed.to_string() + "\""
                                : text(&fixed)) +
             (truthy(at(first, "required")) ? ""
                                            : ", or " + name + " is left out");
    }
  }

  if (!properties.empty() &&
      std::all_of(properties.cbegin(), properties.cend(),
                  [](const auto *property) {
                    return !truthy(at(property, "value")) &&
                           truthy(at(property, "required"));
                  })) {
    std::vector<std::string> names;
    names.reserve(properties.size());
    for (const auto *property : properties) {
      names.push_back(text(property, "name"));
    }

    return "when " + sourcemeta::core::join(names, " and ") + " " +
           (names.size() > 1 ? "are" : "is") + " filled in";
  }

  if (!properties.empty()) {
    std::vector<std::string> names;
    names.reserve(properties.size());
    for (const auto *property : properties) {
      names.push_back(text(property, "name"));
    }

    return "depending on " + sourcemeta::core::join(names, " and ");
  }

  return "in one case";
}

// The property a condition turns on when it only asks whether that one
// property is there, as in "when shippingMethod is filled in".
auto presence_of(const JSON *condition) -> std::optional<std::string> {
  const auto *when{at(condition, "when")};
  if (kind(when) != "object" || count(when, "properties") != 1 ||
      !only_keys(when, {"kind", "properties"})) {
    return std::nullopt;
  }

  const auto *first{list(when, "properties").front()};
  if (truthy(at(first, "required")) && !truthy(at(first, "value"))) {
    return text(first, "name");
  }

  return std::nullopt;
}

// What a value must not be, as phrases.
auto not_phrases(const JSON *value, const Draw &draw)
    -> std::vector<std::string> {
  if (!truthy(value)) {
    return {};
  }

  if (kind(value) == "choice" && truthy(at(value, "overlap"))) {
    std::vector<std::string> result;
    for (const auto *option : list(value, "options")) {
      for (auto &phrase : not_phrases(option, draw)) {
        result.push_back(std::move(phrase));
      }
    }

    return result;
  }

  if (kind(value) == "enum") {
    return {"must not be " + plain_all(at(value, "values"), " or ")};
  }

  const auto properties{list(value, "properties")};
  const auto filled{kind(value) == "object" && !properties.empty() &&
                    std::all_of(properties.cbegin(), properties.cend(),
                                [](const auto *property) {
                                  return truthy(at(property, "required")) &&
                                         !truthy(at(property, "value"));
                                }) &&
                    only_keys(value, {"kind", "properties", "otherTypesAllowed",
                                      "title", "description"})};
  const auto why{truthy(at(value, "description"))
                     ? " (" + text(value, "description") + ")"
                     : std::string{}};
  if (filled) {
    std::vector<std::string> names;
    names.reserve(properties.size());
    for (const auto *property : properties) {
      names.push_back(text(property, "name"));
    }

    return {(names.size() > 1
                 ? "you cannot use " + sourcemeta::core::join(names, " and ") +
                       " together"
                 : names.front() + " may not be used") +
            why};
  }

  const auto facts{facts_of(value)};
  return {"must not be " + label(value, draw) +
          (facts.empty() ? "" : " " + facts) + why};
}

// The names an entry of `patternProperties` is about.
auto pattern_name(const JSON *entry) -> std::string {
  auto result{given(at(entry, "pattern"))
                  ? "any name matching " + text(entry, "pattern")
                  : std::string{"any other name"}};
  const auto *except{at(entry, "except")};
  if (except == nullptr) {
    return result;
  }

  std::vector<std::string> left_out;
  for (const auto *name : list(except, "names")) {
    left_out.push_back(text(name));
  }

  for (const auto *pattern : list(except, "patterns")) {
    left_out.push_back("names matching " + text(pattern));
  }

  return result + ", apart from " + sourcemeta::core::join(left_out, ", ");
}

// Words that take no article: "each one text", not "each one a text".
auto with_article(const std::string &words) -> std::string {
  static const std::vector<std::string> BARE{
      "a",       "an",       "the",     "this", "one",      "either",
      "exactly", "anything", "nothing", "text", "yes or no"};
  std::string lower{words};
  sourcemeta::core::to_lowercase(lower);

  for (const auto &bare : BARE) {
    if (!lower.starts_with(bare)) {
      continue;
    }

    if (lower.size() == bare.size()) {
      return words;
    }

    const auto next{lower[bare.size()]};
    const auto word{sourcemeta::core::is_alphanum(next) || next == '_'};
    if (!word) {
      return words;
    }
  }

  const auto vowel{!lower.empty() &&
                   std::string_view{"aeiou"}.find(lower.front()) !=
                       std::string_view::npos};
  return (vowel ? "an " : "a ") + words;
}

auto capitalised(const std::string &sentence) -> std::string {
  if (sentence.empty()) {
    return sentence;
  }

  auto result{sentence};
  result[0] =
      static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
  return result;
}

// A value said briefly, naming a fixed value outright.
auto brief(const JSON *value, const Draw &draw) -> std::string {
  if (truthy(value) && kind(value) == "enum") {
    const auto *values{at(value, "values")};
    if (count(value, "values") == 1) {
      return "the value " + plain(&values->at(0));
    }

    return "one of " + plain_all(values, ", ");
  }

  return with_article(label(value, draw));
}

// A choice between one group of properties and a few plain values, such as a
// schema that may be an object or a boolean, is drawn as the group with the
// other types noted beside it.
auto as_group(const JSON *value, Draw &draw) -> std::optional<Group> {
  if (value == nullptr) {
    return std::nullopt;
  }

  if (kind(value) == "object") {
    return Group{.group = value, .alternatives = {}};
  }

  if (kind(value) != "choice") {
    return std::nullopt;
  }

  std::vector<const JSON *> objects;
  std::vector<const JSON *> rest;
  for (const auto *option : list(value, "options")) {
    if (option == nullptr || option->is_null()) {
      continue;
    }

    (kind(option) == "object" ? objects : rest).push_back(option);
  }

  if (objects.size() != 1 ||
      !std::all_of(rest.cbegin(), rest.cend(), [](const auto *option) {
        return is_simple(option) && !truthy(at(option, "description"));
      })) {
    return std::nullopt;
  }

  // What the choice itself also says applies to the group too.
  auto group{*objects.front()};
  for (const std::string key : {"also", "conditions"}) {
    const auto *more{at(value, key)};
    if (!truthy(more)) {
      continue;
    }

    auto merged{JSON::make_array()};
    for (const auto *entry : list(&group, key)) {
      merged.push_back(*entry);
    }

    if (more->is_array()) {
      for (const auto &entry : more->as_array()) {
        merged.push_back(entry);
      }
    }

    group.assign(key, std::move(merged));
  }

  if (const auto *forbidden{at(value, "not")}; truthy(forbidden)) {
    if (truthy(at(&group, "not"))) {
      auto options{JSON::make_array()};
      options.push_back(group.at("not"));
      options.push_back(*forbidden);
      auto choice{JSON::make_object()};
      choice.assign("kind", JSON{std::string{"choice"}});
      choice.assign("options", std::move(options));
      choice.assign("overlap", JSON{true});
      group.assign("not", std::move(choice));
    } else {
      group.assign("not", *forbidden);
    }
  }

  return Group{.group = draw.keep(std::move(group)), .alternatives = rest};
}

// What a branch of a condition, or anything pointed at, holds.
auto content_of(const JSON *value, const Draw &draw) -> const JSON * {
  if (kind(value) == "ref") {
    const auto *found{draw.shape(text(value, "ref"))};
    if (found != nullptr) {
      return at(found, "value");
    }
  }

  return value;
}

// Whether any form of a choice carries the author's words.
auto forms_say(const JSON *value) -> bool {
  const auto options{list(value, "options")};
  return std::any_of(options.cbegin(), options.cend(), [](const auto *option) {
    return truthy(option) &&
           (truthy(at(option, "description")) ||
            (kind(option) == "choice" && forms_say(option)) ||
            (kind(option) == "array" && truthy(at(option, "item")) &&
             truthy(at(at(option, "item"), "description"))));
  });
}

// How one form of a choice is named.
auto form_text(const JSON *option, const Draw &draw) -> std::string {
  const auto *item{at(option, "item")};
  std::string inner;
  if (kind(option) == "array" && truthy(item)) {
    if (truthy(at(item, "description"))) {
      inner = text(item, "description");
    } else if (truthy(at(item, "title"))) {
      inner = text(item, "title");
    }
  }

  const auto each{inner.empty() ? std::string{} : ", each entry: " + inner};
  if (truthy(at(option, "description"))) {
    return text(option, "description") + each;
  }

  if (kind(option) == "choice") {
    std::vector<std::string> parts;
    for (const auto *entry : list(option, "options")) {
      parts.push_back(form_text(entry, draw));
    }

    return sourcemeta::core::join(parts, " or ");
  }

  const auto facts{facts_of(option)};
  return label(option, draw) + (facts.empty() ? "" : ", " + facts) + each;
}

// Drawing. Every element goes through the core HTML writer. A part whose
// emptiness decides what wraps it is drawn on its own writer first and then
// placed, as it is already markup.

// The word for one or many, and with `counted` the number before it too.
auto plural(const std::size_t amount, const std::string &one,
            const std::string &many, const bool counted) -> std::string {
  return (counted ? std::to_string(amount) + " " : std::string{}) +
         (amount == 1 ? one : many);
}

auto used(const JSON *shape) -> std::size_t { return count(shape, "usedAt"); }

} // namespace sourcemeta::blaze
