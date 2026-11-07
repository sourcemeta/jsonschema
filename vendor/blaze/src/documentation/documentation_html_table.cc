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

// The tables of a page: one row per property, a strip for what a row holds,
// and the cells that say what goes in each property.

namespace {

using sourcemeta::blaze::at;
using sourcemeta::blaze::condition_text;
using sourcemeta::blaze::content_of;
using sourcemeta::blaze::count;
using sourcemeta::blaze::Draw;
using sourcemeta::blaze::first_example;
using sourcemeta::blaze::form_text;
using sourcemeta::blaze::given;
using sourcemeta::blaze::Inherited;
using sourcemeta::blaze::kind;
using sourcemeta::blaze::link;
using sourcemeta::blaze::linkable;
using sourcemeta::blaze::list;
using sourcemeta::blaze::NameKind;
using sourcemeta::blaze::nested_of;
using sourcemeta::blaze::not_phrases;
using sourcemeta::blaze::only_keys;
using sourcemeta::blaze::OTHER_NAME;
using sourcemeta::blaze::pattern_name;
using sourcemeta::blaze::piece;
using sourcemeta::blaze::plain;
using sourcemeta::blaze::plural;
using sourcemeta::blaze::render_group;
using sourcemeta::blaze::Row;
using sourcemeta::blaze::row_count;
using sourcemeta::blaze::Rows;
using sourcemeta::blaze::rows_of;
using sourcemeta::blaze::strictly_equal;
using sourcemeta::blaze::table;
using sourcemeta::blaze::text;
using sourcemeta::blaze::truthy;
using sourcemeta::blaze::value_cell;
using sourcemeta::blaze::with_article;
using sourcemeta::blaze::Writer;
using sourcemeta::core::JSON;

// How many of the rows are properties rather than conditions, as a label, or
// nothing when none are.
auto properties_in(const Rows &rows) -> std::string {
  std::size_t amount{0};
  for (const auto &row : rows.own) {
    amount += row.condition ? 0 : 1;
  }

  for (const auto &group : rows.inherited) {
    amount += group.rows.size();
  }

  return amount == 0 ? std::string{}
                     : plural(amount, "property", "properties", true);
}

const JSON &never_value() {
  static const JSON NEVER{sourcemeta::core::parse_json(R"({"kind":"never"})")};
  return NEVER;
}

// The lowest and highest allowed, as a sentence.
auto range_text(const JSON *bounds, const std::string &counts) -> std::string {
  const auto *minimum{at(bounds, "min")};
  const auto *maximum{at(bounds, "max")};
  const auto above{truthy(at(bounds, "minExclusive"))};
  const auto below{truthy(at(bounds, "maxExclusive"))};
  if (given(minimum) && given(maximum) && !above && !below) {
    return "from " + plain(minimum) + " to " + plain(maximum) + counts;
  }

  std::vector<std::string> sides;
  if (given(minimum)) {
    sides.push_back((above ? "more than " : "at least ") + plain(minimum));
  }

  if (given(maximum)) {
    sides.push_back((below ? "less than " : "at most ") + plain(maximum));
  }

  return sourcemeta::core::join(sides, " and ") + counts;
}

auto bar(Writer &out, const JSON *bounds, const std::string &counts) -> void {
  out.div().text(range_text(bounds, counts)).close();
}

auto chips(Writer &out, const JSON *values, const JSON *fallback) -> void {
  const auto all{piece([&](Writer &inner) {
    inner.ul();
    if (values != nullptr && values->is_array()) {
      for (const auto &value : values->as_array()) {
        inner.li(strictly_equal(&value, fallback) ? plain(&value) + " (default)"
                                                  : plain(&value));
      }
    }

    inner.close();
  })};

  const auto total{
      values != nullptr && values->is_array() ? values->array_size() : 0};
  if (total <= 6) {
    out.raw(all);
    return;
  }

  out.details();
  out.summary("one of " + std::to_string(total));
  out.raw(all);
  out.close();
}

auto row_name(const Row &row) -> std::string {
  if (row.condition) {
    return condition_text(row.about);
  }

  if (row.name_kind == NameKind::Pattern) {
    return pattern_name(row.about);
  }

  if (row.name_kind == NameKind::Other) {
    return std::string{OTHER_NAME};
  }

  return row.has_name ? row.name : "undefined";
}

auto name_cell(Writer &out, const Row &row) -> void { out.text(row_name(row)); }

auto starts_with_word(const std::string &sentence,
                      const std::vector<std::string> &words) -> bool {
  for (const auto &word : words) {
    if (!sentence.starts_with(word)) {
      continue;
    }

    if (sentence.size() == word.size()) {
      return true;
    }

    const auto next{sentence[word.size()]};
    if (!((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') ||
          (next >= '0' && next <= '9') || next == '_')) {
      return true;
    }
  }

  return false;
}

auto required_cell(Writer &out, const Row &row,
                   const std::optional<std::string> &under) -> void {
  if (row.condition) {
    out.text("Only in certain cases");
  } else if (row.required && under.has_value() && !under.value().empty()) {
    out.text("Yes, " +
             (starts_with_word(under.value(), {"when", "depending", "otherwise",
                                               "in one case"})
                  ? under.value()
                  : "once you fill in " + under.value()));
  } else if (row.required) {
    out.text("Yes, always");
  } else {
    out.text("No, you can leave it out");
  }
}

auto either(Writer &out, const std::vector<const JSON *> &options,
            const Draw &draw) -> void {
  for (std::size_t index = 0; index < options.size(); index++) {
    if (index > 0) {
      // A string rather than one of two literals, which GCC would otherwise
      // hold to the bounds of the shorter one when escaping it.
      const std::string separator{index + 1 == options.size() ? " or " : ", "};
      out.text(separator);
    }

    link(out, text(options[index], "ref"), draw);
  }
}

auto forms_of(Writer &out, const std::vector<const JSON *> &options,
              const Draw &draw) -> void {
  out.ul();
  for (const auto *option : options) {
    if (!truthy(option)) {
      continue;
    }

    out.li();
    const auto *item{at(option, "item")};
    if (kind(option) == "ref") {
      link(out, text(option, "ref"), draw);
      if (truthy(at(option, "description"))) {
        out.text(" ").i(text(option, "description"));
      }
    } else if (kind(option) == "array" && truthy(item) && kind(item) == "ref") {
      out.text("a list, each one ");
      link(out, text(item, "ref"), draw);
    } else {
      out.text(form_text(option, draw));
      const auto *example{first_example(option)};
      if (example != nullptr && !truthy(at(option, "pattern"))) {
        out.text(" ").b(plain(example));
      }

      for (const auto &phrase : not_phrases(at(option, "not"), draw)) {
        out.text(" ").i(phrase);
      }
    }

    out.close();
  }

  out.close();
}

template <typename Inside>
auto notes(Writer &out, const Inside &inside) -> void {
  out.div();
  inside(out);
  out.close();
}

auto external_link(Writer &out, const JSON *value) -> void {
  const auto address{text(value, "href")};
  if (linkable(address)) {
    out.div();
    out.a()
        .attribute("href", address)
        .attribute("rel", "noopener noreferrer")
        .text(address)
        .close();
    out.close();
  } else {
    out.div().text(address).close();
  }
}

// What a value is, as the first line of its cell.
auto typed(Writer &out, const std::string &words) -> void {
  out.div().text(words).close();
}

auto sub_table(const std::string &rows) -> std::string {
  return piece([&](Writer &out) { table(out, rows, true); });
}

// A strip that opens to show what a row holds.
template <typename Title>
auto nest(Writer &out, const Title &title, const std::string &amount,
          const std::string &inner, const std::string &variant = "") -> void {
  out.tr();
  out.td().attribute("colspan", "4");
  out.details();
  if (variant != "dep") {
    out.attribute("open");
  }

  out.summary();
  title(out);
  if (!amount.empty()) {
    out.text(" (" + amount + ")");
  }

  out.close();
  out.raw(inner);
  out.close();
  out.close();
  out.close();
}

// A title made of words with one name in bold.
auto bold_in(const std::string &before, const std::string &name,
             const std::string &after = "") {
  return [before, name, after](Writer &out) {
    out.text(before).b(name).text(after);
  };
}

auto words(const std::string &sentence) {
  return [sentence](Writer &out) { out.text(sentence); };
}

auto leaf(const JSON *value, Draw &draw, const std::string &after = "")
    -> std::string {
  return piece([&](Writer &out) {
           out.div();
           value_cell(out, value, draw);
           out.close();
         }) +
         after;
}

auto render_row(Writer &out, const Row &row, Draw &draw,
                const std::optional<std::string> &under) -> void;

auto has_inside(const JSON *value, const Draw &draw,
                const std::size_t depth = 0) -> bool {
  if (!truthy(value) || depth > 8) {
    return false;
  }

  if (kind(value) == "object") {
    return row_count(rows_of(value, draw)) > 0;
  }

  if (kind(value) == "array") {
    const auto slots{list(value, "slots")};
    return has_inside(at(value, "item"), draw, depth + 1) ||
           std::any_of(slots.cbegin(), slots.cend(),
                       [&draw, depth](const auto *slot) {
                         return has_inside(slot, draw, depth + 1);
                       });
  }

  if (kind(value) == "choice") {
    const auto options{list(value, "options")};
    return std::any_of(options.cbegin(), options.cend(),
                       [&draw, depth](const auto *option) {
                         return has_inside(option, draw, depth + 1);
                       });
  }

  return count(value, "conditions") > 0;
}

auto inherited_summary(const Inherited &inherited, const Draw &draw)
    -> std::string {
  const auto from{draw.name_of(inherited.from)};
  std::size_t properties{0};
  for (const auto &row : inherited.rows) {
    if (!row.condition) {
      properties += 1;
    }
  }

  const auto conditions{inherited.rows.size() - properties};
  const auto conditions_text{conditions == 1
                                 ? std::string{"condition"}
                                 : std::to_string(conditions) + " conditions"};
  if (conditions == 0) {
    return "and the " + std::to_string(properties) + " " +
           plural(properties, "property", "properties") + " every " + from +
           " carries";
  }

  if (properties == 0) {
    return "and the " + conditions_text + " every " + from + " brings";
  }

  return "and the " + std::to_string(properties) + " " +
         plural(properties, "property", "properties") + " and " +
         conditions_text + " every " + from + " brings";
}

// The forms of a choice, each drawn with its properties when it has any.
auto choice_rows(Writer &out, const JSON *choice, Draw &draw,
                 const std::string &name,
                 const std::optional<std::string> &under) -> void {
  const auto options{list(choice, "options")};
  if (!std::any_of(
          options.cbegin(), options.cend(),
          [&draw](const auto *option) { return has_inside(option, draw); })) {
    nest(
        out,
        words(std::string{"and "} +
              (truthy(at(choice, "overlap")) ? "at least one" : "exactly one") +
              " of these must also hold"),
        "", leaf(choice, draw), "dep");
    return;
  }

  for (std::size_t index = 0; index < options.size(); index++) {
    const auto *option{options[index]};
    if (!truthy(option)) {
      continue;
    }

    const auto form{"form " + std::to_string(index + 1) + " of "};
    const auto inside{kind(option) == "object" ? rows_of(option, draw)
                                               : Rows{}};
    const auto amount{row_count(inside)};
    if (amount == 0) {
      const auto inner{piece([&](Writer &writer) {
        nested_of(writer, option, draw, form + name, under);
      })};
      nest(out, bold_in(form, name), "",
           leaf(option, draw, inner.empty() ? "" : sub_table(inner)), "dep");
      continue;
    }

    nest(out,
         bold_in(form, name,
                 truthy(at(option, "description"))
                     ? ": " + text(option, "description")
                     : ""),
         properties_in(inside), sub_table(render_group(option, draw, under)),
         "dep");
  }
}

// Only names that must be filled in, with nothing else said about them.
auto only_requires(const JSON *value) -> bool {
  const auto properties{list(value, "properties")};
  return truthy(value) && kind(value) == "object" && !properties.empty() &&
         std::all_of(properties.cbegin(), properties.cend(),
                     [](const auto *property) {
                       return !truthy(at(property, "value")) &&
                              truthy(at(property, "required"));
                     }) &&
         only_keys(value, {"kind", "properties"});
}

auto render_branch(Writer &out, const JSON *value, Draw &draw,
                   const std::string &lead, const std::string &under) -> void {
  // A shape is written out once, so a branch that is a shape links to it.
  if (kind(value) == "ref" && draw.shape(text(value, "ref")) != nullptr &&
      !only_requires(content_of(value, draw))) {
    const auto reference{text(value, "ref")};
    nest(
        out,
        [&](Writer &title) {
          title.text(lead + " it must also be " +
                     with_article(draw.name_of(reference)));
        },
        "", "", "dep");
    return;
  }

  const auto *body{content_of(value, draw)};
  if (only_requires(body)) {
    const auto properties{list(body, "properties")};
    nest(
        out,
        [&](Writer &title) {
          title.text(lead + " ");
          for (std::size_t index = 0; index < properties.size(); index++) {
            if (index > 0) {
              title.text(" and ");
            }

            title.b(text(properties[index], "name"));
          }

          title.text(properties.size() > 1 ? " become required"
                                           : " becomes required");
        },
        "", "", "dep");
    return;
  }

  if (truthy(body) && kind(body) == "object") {
    const auto inside{rows_of(body, draw)};
    const auto amount{row_count(inside)};
    if (amount > 0) {
      nest(out, words(lead + " these apply"), properties_in(inside),
           sub_table(render_group(body, draw, under)), "dep");
      return;
    }
  }

  nest(out, words(lead + " it must be"), "", leaf(value, draw), "dep");
}

auto render_row(Writer &out, const Row &row, Draw &draw,
                const std::optional<std::string> &under) -> void {
  const auto beneath{under.has_value() && !under.value().empty()};
  const std::string key{row.condition  ? "cond"
                        : row.required ? (beneath ? "cond" : "req")
                                       : "opt"};
  draw.stats[key] += 1;

  if (row.condition) {
    const auto *condition{row.about};
    const auto said{condition_text(condition)};
    const auto *when{at(condition, "when")};
    out.tr();
    out.td();
    name_cell(out, row);
    out.close();
    out.td().close();
    out.td();
    required_cell(out, row, under);
    out.close();
    out.td(truthy(at(when, "description")) ? text(when, "description")
                                           : std::string{});
    out.close();
    if (truthy(at(condition, "then"))) {
      render_branch(out, at(condition, "then"), draw, "then", said);
    }

    if (truthy(at(condition, "otherwise"))) {
      render_branch(out, at(condition, "otherwise"), draw, "otherwise",
                    "otherwise");
    }

    return;
  }

  const auto *value{row.value};
  std::vector<std::string> meaning;
  if (truthy(value)) {
    if (truthy(at(value, "description"))) {
      meaning.push_back(text(value, "description"));
    } else if (truthy(at(value, "title"))) {
      meaning.push_back(text(value, "title"));
    }

    if (truthy(at(value, "deprecated"))) {
      meaning.emplace_back("No longer used.");
    }
  }

  const auto name{row_name(row)};
  out.tr();
  out.td();
  name_cell(out, row);
  out.close();
  out.td();
  value_cell(out, truthy(value) ? value : row.fallback, draw);
  out.close();
  out.td();
  required_cell(out, row, under);
  out.close();
  out.td(sourcemeta::core::join(meaning, " "));
  out.close();
  const auto child_under{
      row.required || beneath ? under : std::optional<std::string>{name}};
  nested_of(out, value, draw, name, child_under);

  for (const auto *then : row.brings) {
    const auto *body{content_of(then, draw)};
    if (only_requires(body)) {
      Writer inner;
      for (const auto *property : list(body, "properties")) {
        inner.tr();
        inner.td(text(property, "name"));
        inner.td().close();
        inner.td("Yes, once you fill in " + name);
        inner.td().close();
        inner.close();
      }

      nest(out,
           bold_in("Filling in ", name,
                   " makes " + std::to_string(count(body, "properties")) +
                       " more required"),
           "", sub_table(inner.take()), "dep");
      continue;
    }

    const auto amount{truthy(body) && kind(body) == "object"
                          ? row_count(rows_of(body, draw))
                          : 0};
    if (amount > 0) {
      nest(out,
           bold_in("Filling in ", name,
                   " brings in " + std::to_string(amount) + " more"),
           "", sub_table(render_group(body, draw, name)), "dep");
    } else {
      nest(out, bold_in("Filling in ", name, " means the whole must also be"),
           "", leaf(then, draw), "dep");
    }
  }
}

} // namespace

namespace sourcemeta::blaze {

auto row_count(const Rows &rows) -> std::size_t {
  auto result{rows.own.size()};
  for (const auto &group : rows.inherited) {
    result += group.rows.size();
  }

  return result;
}

// The rows a group of properties is drawn as: its named properties, the
// properties named by a pattern, the properties not otherwise named, and its
// conditions, in that order. Properties it inherits from a shape come
// separately, grouped by shape.
auto rows_of(const JSON *value, const Draw &draw,
             const std::set<std::string> &seen) -> Rows {
  Rows result;
  if (value == nullptr) {
    return result;
  }

  if (kind(value) != "object") {
    for (const auto *entry : list(value, "conditions")) {
      Row row;
      row.condition = true;
      row.about = entry;
      result.own.push_back(std::move(row));
    }

    return result;
  }

  std::vector<Inherited> inherited;
  for (const auto *also : list(value, "also")) {
    const auto reference{text(also, "ref")};
    if (kind(also) != "ref" || seen.contains(reference)) {
      continue;
    }

    const auto *found{draw.shape(reference)};
    if (found == nullptr || !truthy(at(found, "value")) ||
        kind(at(found, "value")) != "object") {
      continue;
    }

    auto deeper{seen};
    deeper.insert(reference);
    auto inner{rows_of(at(found, "value"), draw, deeper)};
    for (auto &group : inner.inherited) {
      inherited.push_back(std::move(group));
    }

    inherited.push_back({.from = reference, .rows = std::move(inner.own)});
  }

  std::map<std::string, std::pair<std::size_t, std::size_t>> by_name;
  for (std::size_t group = 0; group < inherited.size(); group++) {
    for (std::size_t index = 0; index < inherited[group].rows.size(); index++) {
      const auto &row{inherited[group].rows[index]};
      if (!row.condition && row.name_kind == NameKind::Named && row.has_name) {
        by_name.insert_or_assign(row.name, std::pair{group, index});
      }
    }
  }

  const auto *other{at(value, "otherProperties")};
  const auto *fallback{truthy(other)                 ? other
                       : truthy(at(value, "closed")) ? &never_value()
                                                     : nullptr};
  for (const auto *property : list(value, "properties")) {
    const auto name{text(property, "name")};
    // A property only named as required here, and described by a shape it
    // inherits from, is that inherited property made required.
    const auto match{by_name.find(name)};
    if (!truthy(at(property, "value")) && match != by_name.cend()) {
      auto &row{inherited[match->second.first].rows[match->second.second]};
      row.required = row.required || truthy(at(property, "required"));
      continue;
    }

    Row row;
    row.has_name = at(property, "name") != nullptr;
    if (row.has_name) {
      row.name = name;
    }
    row.value = truthy(at(property, "value")) ? at(property, "value") : nullptr;
    row.required = truthy(at(property, "required"));
    row.fallback = fallback;
    result.own.push_back(std::move(row));
  }

  // A condition that only asks whether one of these properties is there is
  // drawn under that property: "filling in X makes Y required".
  std::vector<const JSON *> conditions;
  for (const auto *entry : list(value, "conditions")) {
    const auto name{presence_of(entry)};
    Row *owner{nullptr};
    if (name.has_value() && !name.value().empty() &&
        !truthy(at(entry, "otherwise"))) {
      for (auto &row : result.own) {
        if (!row.condition && row.has_name && row.name == name.value() &&
            row.name_kind == NameKind::Named) {
          owner = &row;
          break;
        }
      }
    }

    if (owner != nullptr) {
      owner->brings.push_back(at(entry, "then"));
    } else {
      conditions.push_back(entry);
    }
  }

  for (const auto *entry : list(value, "patternProperties")) {
    Row row;
    row.value = truthy(at(entry, "value")) ? at(entry, "value") : nullptr;
    row.name_kind = NameKind::Pattern;
    row.about = entry;
    result.own.push_back(std::move(row));
  }

  if (truthy(other)) {
    Row row;
    row.value = other;
    row.name_kind = NameKind::Other;
    result.own.push_back(std::move(row));
  }

  for (const auto *entry : conditions) {
    Row row;
    row.condition = true;
    row.about = entry;
    result.own.push_back(std::move(row));
  }

  for (auto &group : inherited) {
    if (!group.rows.empty()) {
      result.inherited.push_back(std::move(group));
    }
  }

  return result;
}

// A shape, by its name. As a type, it reads with an article.
auto link(Writer &out, const std::string &reference, const Draw &draw,
          const bool as_type) -> void {
  const auto *target{at(draw.shape(reference), "value")};
  const std::string extra{as_type && truthy(target) && kind(target) == "array"
                              ? ", and that shape is itself a list"
                              : ""};
  out.text((as_type ? with_article(draw.name_of(reference))
                    : draw.name_of(reference)) +
           extra);
}

auto value_cell(Writer &out, const JSON *value, Draw &draw) -> void {
  if (!truthy(value)) {
    typed(out, "no rule beyond being there");
    return;
  }

  const auto name{kind(value)};
  if (name == "ref") {
    link(out, text(value, "ref"), draw);
  } else if (name == "external") {
    typed(out, "follows rules kept in another file");
    external_link(out, value);
  } else if (name == "array") {
    const auto *item{at(value, "item")};
    const auto slots{count(value, "slots")};
    const auto options{list(item, "options")};
    if (slots > 0) {
      typed(out, "a list with " + std::to_string(slots) + " fixed " +
                     (slots == 1 ? "slot" : "slots"));
    } else if (truthy(item) && kind(item) == "ref") {
      out.div().text("a list, each one ");
      link(out, text(item, "ref"), draw);
      out.close();
    } else if (truthy(item) && kind(item) == "choice" && !options.empty() &&
               std::all_of(options.cbegin(), options.cend(),
                           [](const auto *option) {
                             return truthy(option) && kind(option) == "ref";
                           })) {
      out.div().text("a list, each entry either ");
      either(out, options, draw);
      out.close();
    } else if (truthy(item) && kind(item) == "choice" && !options.empty() &&
               (label(item, draw).starts_with("one of") || forms_say(item))) {
      typed(out, "a list, each entry one of " + std::to_string(options.size()) +
                     " forms");
      forms_of(out, options, draw);
    } else {
      std::string said{"a list"};
      if (truthy(item)) {
        std::vector<std::string> parts;
        const auto own{facts_of(item)};
        if (!own.empty()) {
          parts.push_back(own);
        }

        const auto forbidden{
            sourcemeta::core::join(not_phrases(at(item, "not"), draw), "; ")};
        if (!forbidden.empty()) {
          parts.push_back(forbidden);
        }

        said += ", each one " + with_article(label(item, draw));
        if (!parts.empty()) {
          said += ", " + sourcemeta::core::join(parts, ", ");
        }
      }

      typed(out, said);
    }
  } else if (name == "boolean") {
    typed(out, "yes or no");
  } else if (name == "enum") {
    typed(out, count(value, "values") == 1 ? "a fixed value" : "one of these");
    chips(out, at(value, "values"), at(value, "default"));
  } else if (name == "choice") {
    const auto options{list(value, "options")};
    const auto said{label(value, draw)};
    const auto group{as_group(value, draw)};
    if (!options.empty() &&
        std::all_of(options.cbegin(), options.cend(),
                    [](const auto *option) {
                      return truthy(option) && kind(option) == "ref";
                    }) &&
        !forms_say(value)) {
      out.div().text("either ");
      either(out, options, draw);
      out.close();
    } else if (!said.starts_with("one of") && !said.starts_with("either") &&
               !forms_say(value)) {
      typed(out, said);
    } else if (group.has_value()) {
      std::vector<std::string> others;
      for (const auto *alternative : group->alternatives) {
        others.push_back(label(alternative, draw));
      }

      typed(out, label(group->group, draw));
      notes(out, [&](Writer &inner) {
        inner.text("or " + sourcemeta::core::join(others, " or "));
      });
    } else {
      typed(out,
            std::string{truthy(at(value, "overlap")) ? "one or more" : "one"} +
                " of " + std::to_string(options.size()) + " forms");
      forms_of(out, options, draw);
    }
  } else if (name == "object") {
    // A group whose only content is a name the writer picks is a map, and
    // said on the row itself it is one sentence.
    const auto *other{at(value, "otherProperties")};
    const auto patterns{count(value, "patternProperties")};
    const auto only_other{count(value, "properties") == 0 &&
                          count(value, "conditions") == 0 &&
                          ((truthy(other) && patterns == 0) ||
                           (!truthy(other) && patterns == 1))};
    if (only_other) {
      const auto *first{patterns > 0 ? list(value, "patternProperties").front()
                                     : nullptr};
      typed(out, "a group where you choose the names, each one");
      value_cell(out, truthy(other) ? other : at(first, "value"), draw);
      if (!truthy(other)) {
        notes(out, [&](Writer &inner) { inner.text(pattern_name(first)); });
      }
    } else {
      typed(out, label(value, draw));
    }
  } else {
    typed(out, label(value, draw));
  }

  for (const auto *also : list(value, "also")) {
    const auto also_kind{kind(also)};
    if (also_kind == "ref") {
      const auto *found{draw.shape(text(also, "ref"))};
      // Properties inherited from a group of properties are drawn with the
      // properties.
      if (name == "object" && found != nullptr && truthy(at(found, "value")) &&
          kind(at(found, "value")) == "object") {
        continue;
      }

      notes(out, [&](Writer &inner) {
        inner.text("and follows " +
                   with_article(draw.name_of(text(also, "ref"))) + " as well");
      });
    } else if (also_kind == "external") {
      notes(out, [](Writer &inner) {
        inner.text("and follows rules kept in another file as well");
      });
      external_link(out, also);
    } else if (also_kind == "choice" && count(also, "options") > 0) {
      const auto options{list(also, "options")};
      if (label(also, draw).starts_with("one of") &&
          std::any_of(options.cbegin(), options.cend(), [](const auto *option) {
            return truthy(option) && kind(option) == "object";
          })) {
        continue;
      }

      std::vector<std::string> parts;
      parts.reserve(options.size());
      for (const auto *option : options) {
        parts.push_back(truthy(at(option, "description"))
                            ? text(option, "description")
                            : form_text(option, draw));
      }

      notes(out, [&](Writer &inner) {
        inner.text(
            std::string{"and "} +
            (truthy(at(also, "overlap")) ? "at least one" : "exactly one") +
            " of these: " + sourcemeta::core::join(parts, "; "));
      });
    } else if (!(also_kind == "object" && name == "object")) {
      std::vector<std::string> extra;
      if (also_kind == "enum" && at(also, "values") != nullptr &&
          at(also, "values")->is_array()) {
        extra.push_back(plain_all(at(also, "values"), ", "));
      }

      if (const auto *range{at(also, "range")}; truthy(range)) {
        if (given(at(range, "min"))) {
          extra.push_back(range_word(range, "min") + " " +
                          plain(at(range, "min")));
        }

        if (given(at(range, "max"))) {
          extra.push_back(range_word(range, "max") + " " +
                          plain(at(range, "max")));
        }
      }

      if (const auto *length{at(also, "length")}; truthy(length)) {
        if (given(at(length, "min"))) {
          extra.push_back("at least " + plain(at(length, "min")) + " long");
        }

        if (given(at(length, "max"))) {
          extra.push_back("at most " + plain(at(length, "max")) + " long");
        }
      }

      const auto said{pattern_note(also)};
      if (said.has_value()) {
        extra.push_back(said.value());
      }

      notes(out, [&](Writer &inner) {
        inner.text(
            "and must also be " + with_article(label(also, draw)) +
            (extra.empty() ? "" : ", " + sourcemeta::core::join(extra, ", ")));
      });
    }
  }

  const auto *bounds{truthy(at(value, "range")) ? at(value, "range")
                                                : at(value, "length")};
  if (truthy(bounds) && name != "object") {
    const auto *last{given(at(bounds, "max")) ? at(bounds, "max")
                                              : at(bounds, "min")};
    bar(out, bounds, count_words(name, is_one(last)));
  }

  std::vector<std::string> said;
  const auto *example{first_example(value)};
  if (example == nullptr && (name == "ref" || name == "choice")) {
    std::vector<std::string> targets;
    if (name == "ref") {
      targets.push_back(text(value, "ref"));
    } else {
      for (const auto *option : list(value, "options")) {
        if (truthy(option) && kind(option) == "ref") {
          targets.push_back(text(option, "ref"));
        }
      }
    }

    for (const auto &target : targets) {
      const auto *found{draw.shape(target)};
      const auto *other{found == nullptr ? nullptr
                                         : first_example(at(found, "value"))};
      if (other != nullptr) {
        example = other;
        break;
      }
    }
  }

  const auto pattern{pattern_note(at(value, "pattern"), example)};
  if (pattern.has_value()) {
    said.push_back(pattern.value());
  }

  if (given(at(value, "multipleOf"))) {
    said.push_back("in steps of " + plain(at(value, "multipleOf")));
  }

  const auto *item{at(value, "item")};
  const auto item_pattern{pattern_note(item)};
  if (item_pattern.has_value()) {
    said.push_back("each entry " + item_pattern.value());
  }

  if (!pattern.has_value() && !item_pattern.has_value() && example != nullptr &&
      name != "enum") {
    said.push_back("for example " + plain(example));
  }

  // What the author wrote about a single entry, when a list is drawn on one
  // row.
  if (truthy(item)) {
    if (truthy(at(item, "description"))) {
      said.push_back("each entry: " + text(item, "description"));
    } else if (truthy(at(item, "title"))) {
      said.push_back("each entry: " + text(item, "title"));
    }
  }

  for (auto &phrase : not_phrases(at(value, "not"), draw)) {
    said.push_back(std::move(phrase));
  }

  if (const auto *amount{at(value, "propertyCount")}; truthy(amount)) {
    const auto *minimum{at(amount, "min")};
    const auto *maximum{at(amount, "max")};
    if (given(minimum) && given(maximum)) {
      said.push_back("between " + text(minimum) + " and " + text(maximum) +
                     " properties");
    } else if (given(minimum)) {
      said.push_back("at least " + text(minimum) + " " +
                     (is_one(minimum) ? "property" : "properties"));
    } else {
      said.push_back("at most " + text(maximum) + " " +
                     (is_one(maximum) ? "property" : "properties"));
    }
  }

  if (const auto *keys{at(value, "keys")}; truthy(keys)) {
    if (truthy(at(keys, "format"))) {
      said.push_back("every name must be " + text(keys, "format"));
    }

    if (truthy(at(keys, "pattern"))) {
      said.push_back("every name must match " + text(keys, "pattern"));
    } else if (kind(keys) == "enum") {
      said.push_back("names can only be " +
                     plain_all(at(keys, "values"), ", "));
    } else if (kind(keys) != "string") {
      said.push_back("every name must be " + with_article(label(keys, draw)));
    }

    if (const auto *length{at(keys, "length")}; truthy(length)) {
      said.push_back("names from " +
                     (given(at(length, "min")) ? text(at(length, "min"))
                                               : std::string{"any"}) +
                     " to " +
                     (given(at(length, "max")) ? text(at(length, "max"))
                                               : std::string{"any number of"}) +
                     " characters");
    }
  }

  if (const auto *contains{at(value, "contains")}; truthy(contains)) {
    const auto *minimum{at(contains, "min")};
    const auto *maximum{at(contains, "max")};
    const auto how{given(maximum)
                       ? "between " + text(minimum) + " and " + text(maximum)
                       : "at least " + text(minimum)};
    const auto facts{facts_of(at(contains, "value"))};
    said.push_back(how + " " +
                   (is_one(minimum) && !given(maximum) ? "entry" : "entries") +
                   " must be " + brief(at(contains, "value"), draw) +
                   (facts.empty() ? "" : ", " + facts));
  }

  if (const auto *access{at(value, "access")}; access != nullptr) {
    if (text(access) == "read") {
      said.emplace_back("you do not fill this in, it is sent back to you");
    } else if (text(access) == "write") {
      said.emplace_back("you fill this in, it is never sent back");
    }
  }

  if (const auto *content{at(value, "content")}; truthy(content)) {
    said.push_back("the text inside is " +
                   (truthy(at(content, "mediaType"))
                        ? text(content, "mediaType")
                        : std::string{"in a set format"}) +
                   (truthy(at(content, "encoding"))
                        ? ", written as " + text(content, "encoding")
                        : ""));
  }

  if (truthy(at(value, "unique"))) {
    said.emplace_back("no two entries the same");
  }

  if (truthy(at(value, "closed"))) {
    said.emplace_back("nothing else may be added here");
  }

  if (truthy(item) && truthy(at(item, "closed"))) {
    said.emplace_back("nothing else may be added to an entry");
  }

  if (truthy(at(value, "nullable"))) {
    said.emplace_back("it may also be null");
  }

  if (truthy(at(value, "otherTypesAllowed"))) {
    said.emplace_back(count(value, "also") > 0 || truthy(at(value, "not")) ||
                              count(value, "conditions") > 0
                          ? "another kind of value passes the rules above, "
                            "and still has to match the rest"
                          : "any other kind of value is also accepted");
  }

  if (at(value, "default") != nullptr && name != "enum") {
    said.push_back("left out means " + plain(at(value, "default")));
  }

  std::erase_if(said, [](const auto &note) { return note.empty(); });
  for (const auto &note : said) {
    out.div(note);
  }
}

// A table of rows, or nothing when there are no rows.
auto table(Writer &out, const std::string &rows, const bool sub) -> void {
  if (rows.empty()) {
    return;
  }

  out.table();
  if (!sub) {
    out.tr();
    out.th("Property");
    out.th("What goes in it");
    out.th("Required?");
    out.th("What it means");
    out.close();
  }

  out.raw(rows);
  out.close();
}

// Every row of a group, and the inherited ones behind a strip.
auto render_group(const JSON *value, Draw &draw,
                  const std::optional<std::string> &under) -> std::string {
  const auto rows{rows_of(value, draw)};
  Writer out;
  for (const auto &row : rows.own) {
    render_row(out, row, draw, under);
  }

  for (const auto &inherited : rows.inherited) {
    out.tr();
    out.td().attribute("colspan", "4");
    out.details();
    out.summary().text(inherited_summary(inherited, draw));
    // A shape can inherit, somewhere inside itself, from itself. Its
    // properties are then already being drawn, so the strip links to them.
    if (draw.inlining.contains(inherited.from)) {
      out.text(", as written out in ");
      link(out, inherited.from, draw, false);
      out.close();
    } else {
      out.close();
      draw.inlining.insert(inherited.from);
      Writer inner;
      for (const auto &row : inherited.rows) {
        render_row(inner, row, draw, under);
      }

      out.raw(sub_table(inner.take()));
      draw.inlining.erase(inherited.from);
    }

    out.close();
    out.close();
    out.close();
  }

  // A choice the whole group must also make: which forms it may take.
  for (const auto *also : list(value, "also")) {
    if (kind(also) == "choice") {
      choice_rows(out, also, draw, "it", under);
    }
  }

  // Properties written out in a part that also applies, kept apart because
  // they could not be merged with these.
  for (const auto *also : list(value, "also")) {
    if (kind(also) != "object") {
      continue;
    }

    const auto inside{rows_of(also, draw)};
    const auto amount{row_count(inside)};
    if (amount > 0) {
      nest(out, words("and it must also match these"), properties_in(inside),
           sub_table(render_group(also, draw, under)), "dep");
    }
  }

  return out.take();
}

// A group made only of names the writer chooses is said on its row.
auto is_map(const JSON *value) -> bool {
  const auto *other{at(value, "otherProperties")};
  const auto patterns{count(value, "patternProperties")};
  return count(value, "properties") == 0 && count(value, "conditions") == 0 &&
         count(value, "also") == 0 &&
         ((truthy(other) && patterns == 0) ||
          (!truthy(other) && patterns == 1));
}

// Whether the properties under a map's chosen names have anything to draw.
auto map_has_inside(const JSON *value, const Draw &draw) -> bool {
  const auto *other{at(value, "otherProperties")};
  const auto *inner{
      truthy(other) ? other
                    : at(list(value, "patternProperties").front(), "value")};
  return has_inside(inner, draw) ||
         (kind(inner) == "choice" && forms_say(inner)) ||
         truthy(at(inner, "description")) ||
         (truthy(at(inner, "item")) &&
          truthy(at(at(inner, "item"), "description")));
}

// What is drawn inside a row: the properties of a group, of each entry of a
// list, of each form of a choice, and the fixed slots of a list.
auto nested_of(Writer &out, const JSON *value, Draw &draw,
               const std::string &name, const std::optional<std::string> &under)
    -> void {
  if (!truthy(value)) {
    return;
  }

  // Text that holds a document of its own, with rules of its own.
  if (const auto *content{at(value, "content")};
      truthy(content) && truthy(at(content, "value"))) {
    const auto *inner{at(content, "value")};
    const auto inside{kind(inner) == "object" ? rows_of(inner, draw) : Rows{}};
    const auto amount{row_count(inside)};
    if (amount > 0) {
      nest(out, bold_in("the document inside ", name), properties_in(inside),
           sub_table(render_group(inner, draw, std::nullopt)), "dep");
    } else {
      nest(out, bold_in("the document inside ", name, " is"), "",
           leaf(inner, draw), "dep");
    }
  }

  const auto group{as_group(value, draw)};
  std::optional<Rows> rows;
  if (group.has_value()) {
    rows = rows_of(group->group, draw);
  } else if (count(value, "conditions") > 0) {
    rows = rows_of(value, draw);
  }

  const auto *item{at(value, "item")};
  if (rows.has_value() && row_count(rows.value()) > 0 &&
      !(group.has_value() && is_map(group->group) &&
        !map_has_inside(group->group, draw))) {
    nest(out, bold_in("inside ", name), properties_in(rows.value()),
         sub_table(render_group(group.has_value() ? group->group : value, draw,
                                under)));
  } else if (kind(value) == "array" && truthy(item) && kind(item) == "object" &&
             row_count(rows_of(item, draw)) > 0) {
    nest(out, bold_in("inside ", "each entry of " + name),
         properties_in(rows_of(item, draw)),
         sub_table(render_group(item, draw, under)));
  } else if (kind(value) == "array" && truthy(item) && kind(item) == "choice") {
    const auto options{list(item, "options")};
    if (std::any_of(
            options.cbegin(), options.cend(),
            [&draw](const auto *option) { return has_inside(option, draw); })) {
      choice_rows(out, item, draw, "each entry of " + name, under);
    }
  }

  if (!group.has_value() && kind(value) == "choice") {
    const auto options{list(value, "options")};
    if (std::any_of(
            options.cbegin(), options.cend(),
            [&draw](const auto *option) { return has_inside(option, draw); })) {
      choice_rows(out, value, draw, name, under);
    }
  }

  // A value that is not itself a group, but must also match one.
  if (!group.has_value()) {
    for (const auto *also : list(value, "also")) {
      if (kind(also) != "object") {
        continue;
      }

      const auto inside{rows_of(also, draw)};
      const auto amount{row_count(inside)};
      if (amount > 0) {
        nest(out, words("and it must also match these"), properties_in(inside),
             sub_table(render_group(also, draw, under)), "dep");
      }
    }
  }

  const auto slots{list(value, "slots")};
  if (kind(value) == "array" && !slots.empty()) {
    Writer slot_rows;
    for (std::size_t index = 0; index < slots.size(); index++) {
      Row row;
      row.name = "slot " + std::to_string(index + 1);
      row.has_name = true;
      row.value = truthy(slots[index]) ? slots[index] : nullptr;
      render_row(slot_rows, row, draw, under);
    }

    nest(out, bold_in("the fixed slots of ", name),
         plural(slots.size(), "slot", "slots", true),
         sub_table(slot_rows.take()));
    if (truthy(item) && kind(item) == "never") {
      nest(out, bold_in("after those slots, ", name, " holds nothing more"), "",
           "", "dep");
    } else if (truthy(item)) {
      Row row;
      row.name = "every entry after those";
      row.has_name = true;
      row.value = item;
      Writer rest;
      render_row(rest, row, draw, under);
      nest(out, bold_in("what ", name, " holds after the fixed slots"), "",
           sub_table(rest.take()), "dep");
    }
  }
}

} // namespace sourcemeta::blaze
