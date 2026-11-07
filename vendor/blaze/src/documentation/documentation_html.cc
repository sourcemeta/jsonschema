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

// The table format into a page. This renderer reads the table format and
// nothing else, so it draws a page for a schema of any language that was
// converted into it. It writes bare markup with no stylesheet, no classes and
// no ids, so whoever shows the page wraps it in an element of their own and
// styles it from there. It holds:
//
// - the shapes everything else is built on, before the things built on them
// - one row per property, with its limits written out
// - a thing inside a thing is a table inside that row
// - inherited properties behind one disclosure, opened in place
// - a shape is named wherever it is used, and written out once
//
// No scripting, no JSON on the page.

namespace {

using sourcemeta::blaze::as_group;
using sourcemeta::blaze::at;
using sourcemeta::blaze::capitalised;
using sourcemeta::blaze::count;
using sourcemeta::blaze::Draw;
using sourcemeta::blaze::is_map;
using sourcemeta::blaze::kind;
using sourcemeta::blaze::label;
using sourcemeta::blaze::link;
using sourcemeta::blaze::list;
using sourcemeta::blaze::map_has_inside;
using sourcemeta::blaze::nested_of;
using sourcemeta::blaze::not_phrases;
using sourcemeta::blaze::plain;
using sourcemeta::blaze::plural;
using sourcemeta::blaze::render_group;
using sourcemeta::blaze::row_count;
using sourcemeta::blaze::Rows;
using sourcemeta::blaze::rows_of;
using sourcemeta::blaze::shape_name;
using sourcemeta::blaze::table;
using sourcemeta::blaze::text;
using sourcemeta::blaze::truthy;
using sourcemeta::blaze::used;
using sourcemeta::blaze::value_cell;
using sourcemeta::blaze::Writer;
using sourcemeta::core::JSON;

// The tags that start a block of the page. A line break next to one of them
// does not change how the page looks, while one next to an inline tag, like a
// `span`, would put a space where there was none.
auto is_block(const std::string_view name) -> bool {
  static constexpr std::array<std::string_view, 15> BLOCKS{
      {"div", "section", "h1", "h2", "h3", "h4", "p", "table", "tr", "th", "td",
       "ul", "li", "details", "summary"}};
  return std::find(BLOCKS.cbegin(), BLOCKS.cend(), name) != BLOCKS.cend();
}

auto break_line(std::string &output, const std::size_t depth) -> void {
  if (!output.empty()) {
    output += '\n';
    output.append(depth * 2, ' ');
  }
}

// Put every block of a page on a line of its own, indented by how deep it is,
// and leave what is inside a line as it was. Every attribute value on the page
// is escaped, so a tag ends at the first `>` after it starts.
auto indented(const std::string_view page) -> std::string {
  std::string output;
  output.reserve(page.size() + (page.size() / 2));
  // Whether each block still open holds another block, which then puts its
  // closing tag on a line of its own too
  std::vector<bool> holds;
  std::size_t cursor{0};
  while (cursor < page.size()) {
    if (page[cursor] != '<') {
      const auto next{page.find('<', cursor)};
      const auto end{next == std::string_view::npos ? page.size() : next};
      output.append(page.substr(cursor, end - cursor));
      cursor = end;
      continue;
    }

    const auto end{page.find('>', cursor) + 1};
    const auto tag{page.substr(cursor, end - cursor)};
    cursor = end;
    const auto closing{tag.starts_with("</")};
    const auto start{closing ? std::size_t{2} : std::size_t{1}};
    const auto name{tag.substr(start, tag.find_first_of(" />", start) - start)};
    if (!is_block(name)) {
      output.append(tag);
    } else if (closing) {
      if (holds.back()) {
        break_line(output, holds.size() - 1);
      }

      holds.pop_back();
      output.append(tag);
    } else {
      if (!holds.empty()) {
        holds.back() = true;
      }

      break_line(output, holds.size());
      output.append(tag);
      if (!tag.ends_with("/>")) {
        holds.push_back(false);
      }
    }
  }

  output += '\n';
  return output;
}

// A shape, written out once.
auto shape_block(Writer &out, const JSON *shape, Draw &draw) -> void {
  const auto *value{at(shape, "value")};
  const auto list_shape{kind(value) == "array"};
  const auto group{as_group(list_shape ? at(value, "item") : value, draw)};
  const auto *shape_body{group.has_value() ? group->group : value};
  std::optional<Rows> rows;
  if (group.has_value() &&
      !(is_map(shape_body) && !map_has_inside(shape_body, draw))) {
    rows = rows_of(shape_body, draw);
  } else if (count(value, "conditions") > 0) {
    rows = rows_of(value, draw);
  }

  const auto drawn{rows.has_value() && row_count(rows.value()) > 0};
  out.div();
  out.h4(shape_name(shape));
  if (truthy(at(value, "description"))) {
    out.p(text(value, "description"));
  }

  if (group.has_value() && !group->alternatives.empty()) {
    std::vector<std::string> others;
    for (const auto *alternative : group->alternatives) {
      others.push_back(label(alternative, draw));
    }

    out.p("It may instead be " + sourcemeta::core::join(others, " or ") + ".");
  }

  for (const auto &phrase : not_phrases(at(shape_body, "not"), draw)) {
    out.p(capitalised(phrase) + ".");
  }

  for (const auto *also : list(value, "also")) {
    if (kind(also) != "ref") {
      continue;
    }

    const auto *target{draw.shape(text(also, "ref"))};
    if (kind(value) == "object" && target != nullptr &&
        truthy(at(target, "value")) && kind(at(target, "value")) == "object") {
      continue;
    }

    out.p().text("This also follows every rule of ");
    link(out, text(also, "ref"), draw, false);
    out.text(".").close();
  }

  if (const auto *picker{at(shape_body, "discriminator")}; truthy(picker)) {
    std::vector<std::string> cases;
    for (const auto *entry : list(picker, "cases")) {
      cases.push_back(plain(at(entry, "is")) + " means " +
                      draw.name_of(text(entry, "ref")));
    }

    out.p("The property " + text(picker, "property") +
          " decides which shape applies: " +
          sourcemeta::core::join(cases, ", ") + ".");
  }

  if (list_shape) {
    out.p(drawn ? "This one is a list. The properties below describe a "
                  "single entry."
                : "This one is a list.");
  }

  if (truthy(at(shape_body, "closed"))) {
    out.p("Nothing else may be added to it.");
  }

  if (used(shape) > 0) {
    out.p("Used in " + std::to_string(used(shape)) + " " +
          plural(used(shape), "place", "places") + ".");
  }

  if (drawn) {
    table(out, render_group(group.has_value() ? shape_body : value, draw,
                            std::nullopt));
  } else {
    out.div();
    value_cell(out, value, draw);
    out.close();
    Writer inner;
    nested_of(inner, value, draw, shape_name(shape), std::nullopt);
    const auto rows_inside{inner.take()};
    if (!rows_inside.empty()) {
      out.table();
      out.raw(rows_inside);
      out.close();
    }
  }

  out.close();
}

// How many of the properties drawn were of a kind.
auto stat(const Draw &draw, const std::string &key) -> std::size_t {
  const auto match{draw.stats.find(key)};
  return match == draw.stats.cend() ? 0 : match->second;
}

// How many properties there are to fill in, or else how many shapes.
auto counts_sentence(const Draw &draw, const std::size_t shapes)
    -> std::string {
  std::vector<std::string> parts;
  for (const auto &[key, sentence] :
       std::array<std::pair<std::string, std::string>, 3>{
           {{"req", "you must always fill in"},
            {"opt", "you can leave out"},
            {"cond", "that only apply in certain cases"}}}) {
    const auto amount{stat(draw, key)};
    if (amount == 0) {
      continue;
    }

    // One property applies, where many apply.
    const auto said{key == "cond" && amount == 1
                        ? std::string{"that only applies in certain cases"}
                        : sentence};

    // The first number says what it counts, and the rest follow it.
    parts.push_back(parts.empty()
                        ? plural(amount, "property", "properties", true) + " " +
                              said
                        : std::to_string(amount) + " " + said);
  }

  if (parts.empty() && shapes > 0) {
    parts.push_back(std::to_string(shapes) + " " +
                    plural(shapes, "shape", "shapes") + ", written out below");
  }

  return parts.empty() ? std::string{}
                       : sourcemeta::core::join(parts, ", ") + ".";
}

} // namespace

namespace sourcemeta::blaze {

auto to_html(const sourcemeta::core::JSON &documentation, std::string &output,
             const bool pretty) -> void {
  Draw draw;
  const auto *document{&documentation};
  const auto title{text(document, "title")};
  const auto shapes{list(document, "shapes")};

  for (const auto *shape : shapes) {
    draw.shapes.insert_or_assign(text(shape, "id"), shape);
  }

  auto root_shape{JSON::make_object()};
  root_shape.assign("id", JSON{std::string{}});
  root_shape.assign("name", JSON{title});
  if (const auto *root{at(document, "root")}; root != nullptr) {
    root_shape.assign("value", *root);
  }

  draw.shapes.insert_or_assign("", draw.keep(std::move(root_shape)));
  const auto *root{at(document, "root")};

  // The shapes, by the section each belongs to, in the order they come.
  std::vector<std::pair<std::string, std::vector<const JSON *>>> groups;
  for (const auto *shape : shapes) {
    const auto section{truthy(at(shape, "section")) ? text(shape, "section")
                                                    : std::string{"Shapes"}};
    auto match{std::find_if(
        groups.begin(), groups.end(),
        [&section](const auto &entry) { return entry.first == section; })};
    if (match == groups.end()) {
      groups.emplace_back(section, std::vector<const JSON *>{});
      match = std::prev(groups.end());
    }

    match->second.push_back(shape);
  }

  Writer body;

  // Which shape applies, when one property decides it.
  const auto root_group{as_group(root, draw)};
  const auto *top{root_group.has_value() ? root_group->group : root};
  const auto root_rows{root_group.has_value()
                           ? render_group(top, draw, std::nullopt)
                           : std::string{}};
  if (const auto *picker{at(top, "discriminator")}; truthy(picker)) {
    const auto cases{list(picker, "cases")};
    body.section();
    body.h2("Which kind of thing this is");
    body.p().text("The property ");
    body.b(text(picker, "property"));
    body.text(" decides which shape applies. There " +
              (cases.size() == 1
                   ? std::string{"is one choice"}
                   : "are " + std::to_string(cases.size()) + " choices") +
              ".");
    body.close();
    body.ul();
    for (const auto *entry : cases) {
      body.li(plain(at(entry, "is")) + ": " + draw.name_of(text(entry, "ref")));
    }

    body.close();
    body.close();
  }

  std::vector<std::string> root_notes;
  if (root_group.has_value() && !root_group->alternatives.empty()) {
    std::vector<std::string> others;
    for (const auto *alternative : root_group->alternatives) {
      others.push_back(label(alternative, draw));
    }

    root_notes.push_back("The whole file may instead be " +
                         sourcemeta::core::join(others, " or ") + ".");
  }

  if (truthy(at(top, "closed"))) {
    root_notes.emplace_back("Nothing else may be added at the top level.");
  }

  for (const auto &phrase : not_phrases(at(top, "not"), draw)) {
    root_notes.push_back(capitalised(phrase) + ".");
  }

  if (const auto *amount{at(top, "propertyCount")}; truthy(amount)) {
    const auto *minimum{at(amount, "min")};
    const auto *maximum{at(amount, "max")};
    if (given(minimum) && given(maximum)) {
      root_notes.push_back("Between " + text(minimum) + " and " +
                           text(maximum) + " properties in all.");
    } else if (given(minimum)) {
      root_notes.push_back("At least " + text(minimum) + " " +
                           (is_one(minimum) ? "property" : "properties") +
                           " in all.");
    } else {
      root_notes.push_back("At most " + text(maximum) + " " +
                           (is_one(maximum) ? "property" : "properties") +
                           " in all.");
    }
  }

  // What a property's own row would say about the top level, which a table of
  // properties has nowhere to put.
  if (const auto *keys{at(top, "keys")}; truthy(keys)) {
    if (truthy(at(keys, "pattern"))) {
      root_notes.push_back("Every property name must match " +
                           text(keys, "pattern") + ".");
    } else if (kind(keys) == "enum" && at(keys, "values") != nullptr &&
               at(keys, "values")->is_array()) {
      root_notes.push_back("Property names can only be " +
                           plain_all(at(keys, "values"), ", ") + ".");
    } else if (truthy(at(keys, "format"))) {
      root_notes.push_back("Every property name must be " +
                           text(keys, "format") + ".");
    }

    if (const auto *length{at(keys, "length")}; truthy(length)) {
      const auto low{given(at(length, "min"))};
      const auto high{given(at(length, "max"))};
      root_notes.push_back(
          "Property names are " +
          (low ? "at least " + text(at(length, "min")) : std::string{}) +
          (low && high ? " and " : "") +
          (high ? "at most " + text(at(length, "max")) : std::string{}) +
          " characters.");
    }
  }

  if (truthy(at(top, "unique"))) {
    root_notes.emplace_back("No two entries are the same.");
  }

  if (given(at(top, "multipleOf"))) {
    root_notes.push_back("In steps of " + plain(at(top, "multipleOf")) + ".");
  }

  if (at(top, "default") != nullptr) {
    root_notes.push_back("Left out means " + plain(at(top, "default")) + ".");
  }

  if (truthy(at(top, "access"))) {
    root_notes.emplace_back(text(top, "access") == "read"
                                ? "The system supplies this; you do not "
                                  "write it."
                                : "You write this; it is never returned.");
  }

  if (truthy(at(top, "otherTypesAllowed"))) {
    root_notes.emplace_back("Data of another kind passes these rules too.");
  }

  if (const auto *content{at(top, "content")};
      truthy(content) && truthy(at(content, "mediaType"))) {
    root_notes.push_back("The text holds " + text(content, "mediaType") + ".");
  }

  body.section();
  body.h2("What this file holds");
  if (!root_rows.empty()) {
    for (const auto &note : root_notes) {
      body.p(note);
    }

    table(body, root_rows);
  } else {
    body.div();
    value_cell(body, root, draw);
    body.close();
    Writer inner;
    nested_of(inner, root, draw, title, std::nullopt);
    const auto rows{inner.take()};
    if (!rows.empty()) {
      body.table();
      body.raw(rows);
      body.close();
    }
  }

  body.close();

  if (shapes.size() > 8) {
    body.section();
    body.h2("All " + std::to_string(shapes.size()) + " shapes");
    body.p("Every shape this document describes. The ones used most often come "
           "first, because everything else is built on them.");
    for (const auto &[section, members] : groups) {
      if (groups.size() > 1) {
        body.h3(section);
      }

      body.ul();
      for (const auto *shape : members) {
        body.li(used(shape) > 0 ? shape_name(shape) + " (used " +
                                      std::to_string(used(shape)) + ")"
                                : shape_name(shape));
      }

      body.close();
    }

    body.close();
  }

  if (!shapes.empty()) {
    body.section();
    body.h2("Every shape, written out once");
    body.p("Each shape appears here once. Anywhere else on this page that "
           "names one, it means exactly this.");
    for (const auto &[section, members] : groups) {
      if (groups.size() > 1) {
        body.h3(section);
      }

      for (const auto *shape : members) {
        shape_block(body, shape, draw);
      }
    }

    body.close();
  }

  const auto drawn{body.take()};

  Writer page;
  page.h1(title);
  if (truthy(at(document, "description"))) {
    page.p(text(document, "description"));
  }

  const auto counts{counts_sentence(draw, shapes.size())};
  if (!counts.empty()) {
    page.p(counts);
  }

  // Each line of the legend is a reading instruction, so it is only given
  // when the page holds the thing it talks about.
  if (drawn.find("<table>") != std::string::npos) {
    page.p()
        .b("A table inside a row")
        .text(" means those properties live inside that one.")
        .close();
  }

  if (drawn.find("<details") != std::string::npos) {
    page.p()
        .b("Click a line with an arrow")
        .text(" to open what it holds.")
        .close();
  }

  page.raw(drawn);
  if (pretty) {
    output += indented(page.str());
  } else {
    output += page.take();
  }
}

} // namespace sourcemeta::blaze
