#ifndef SOURCEMETA_BLAZE_DOCUMENTATION_INTERNAL_HTML_H_
#define SOURCEMETA_BLAZE_DOCUMENTATION_INTERNAL_HTML_H_

#include <sourcemeta/core/html.h>
#include <sourcemeta/core/json.h>

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <deque>       // std::deque
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

// What the three files that draw a page share: the words a page says, in
// `documentation_html_words.cc`, the tables it draws, in
// `documentation_html_table.cc`, and the page, in `documentation_html.cc`.

namespace sourcemeta::blaze {

struct Draw {
  std::map<std::string, const sourcemeta::core::JSON *> shapes;
  std::map<std::string, std::size_t> stats;
  std::set<std::string> inlining;
  // Values put together while drawing, which the page points into.
  std::deque<sourcemeta::core::JSON> kept;

  auto keep(sourcemeta::core::JSON value) -> const sourcemeta::core::JSON * {
    this->kept.push_back(std::move(value));
    return &this->kept.back();
  }

  [[nodiscard]] auto shape(const std::string &identifier) const
      -> const sourcemeta::core::JSON * {
    const auto match{this->shapes.find(identifier)};
    return match == this->shapes.cend() ? nullptr : match->second;
  }

  [[nodiscard]] auto name_of(const std::string &identifier) const
      -> std::string;
};

inline constexpr std::string_view OTHER_NAME{"any name you choose"};

enum class NameKind : std::uint8_t { Named, Pattern, Other };

struct Row {
  bool condition{false};
  // The condition a row stands for, or the `patternProperties` entry.
  const sourcemeta::core::JSON *about{nullptr};
  // A row for a property. A property may be named with empty text, so whether
  // the row has a name is kept apart from the name itself.
  std::string name;
  bool has_name{false};
  const sourcemeta::core::JSON *value{nullptr};
  bool required{false};
  NameKind name_kind{NameKind::Named};
  const sourcemeta::core::JSON *fallback{nullptr};
  std::vector<const sourcemeta::core::JSON *> brings;
};

struct Inherited {
  std::string from;
  std::vector<Row> rows;
};

struct Rows {
  std::vector<Row> own;
  std::vector<Inherited> inherited;
};

struct Group {
  const sourcemeta::core::JSON *group;
  std::vector<const sourcemeta::core::JSON *> alternatives;
};

using Writer = sourcemeta::core::HTMLWriter;

template <typename Drawing> auto piece(const Drawing &drawing) -> std::string {
  Writer writer;
  drawing(writer);
  return writer.take();
}

auto at(const sourcemeta::core::JSON *value, const std::string &key)
    -> const sourcemeta::core::JSON *;

auto truthy(const sourcemeta::core::JSON *value) -> bool;

// Present and not null.
auto given(const sourcemeta::core::JSON *value) -> bool;

auto list(const sourcemeta::core::JSON *value, const std::string &key)
    -> std::vector<const sourcemeta::core::JSON *>;

auto count(const sourcemeta::core::JSON *value, const std::string &key)
    -> std::size_t;

auto kind(const sourcemeta::core::JSON *value) -> std::string;

auto is_one(const sourcemeta::core::JSON *value) -> bool;

auto text(const sourcemeta::core::JSON *value) -> std::string;

auto text(const sourcemeta::core::JSON *value, const std::string &key)
    -> std::string;

// Values a reader sees, never sourcemeta::core::JSON.
auto plain(const sourcemeta::core::JSON *value) -> std::string;

auto plain_all(const sourcemeta::core::JSON *values,
               const std::string_view glue) -> std::string;

// Whether a value is the same as another by the strict equality of the
// reference renderer: scalars by value, anything with parts never.
auto strictly_equal(const sourcemeta::core::JSON *left,
                    const sourcemeta::core::JSON *right) -> bool;

// An address from a schema is shown as a link only when following it is safe.
// A browser throws away tabs, newlines and control characters before it reads
// an address, so they are thrown away here too.
auto linkable(const std::string_view address) -> bool;

// What a length counts, by the kind it is on.
// What a count of a kind of value counts, for one or for many.
auto count_words(const std::string &name, const bool one = false)
    -> std::string;

auto only_keys(const sourcemeta::core::JSON *value,
               const std::set<std::string> &allowed) -> bool;

// What to call a shape: the author's title, or the name it is declared under.
auto shape_name(const sourcemeta::core::JSON *shape) -> std::string;

// The short phrase for a value, worked out from its facts alone.
auto label(const sourcemeta::core::JSON *value, const Draw &draw)
    -> std::string;

// The first example a reader can take in at a glance.
auto first_example(const sourcemeta::core::JSON *value)
    -> const sourcemeta::core::JSON *;

// How a pattern is presented: exactly as written, with an example if there is
// one.
auto pattern_note(const sourcemeta::core::JSON *pattern,
                  const sourcemeta::core::JSON *example)
    -> std::optional<std::string>;

auto pattern_note(const sourcemeta::core::JSON *value)
    -> std::optional<std::string>;

auto range_word(const sourcemeta::core::JSON *bounds, const std::string &edge)
    -> std::string;

auto facts_of(const sourcemeta::core::JSON *value) -> std::string;

// What a condition turns on, read from the value it tests.
auto condition_text(const sourcemeta::core::JSON *condition) -> std::string;

// The property a condition turns on when it only asks whether that one
// property is there, as in "when shippingMethod is filled in".
auto presence_of(const sourcemeta::core::JSON *condition)
    -> std::optional<std::string>;

// What a value must not be, as phrases.
auto not_phrases(const sourcemeta::core::JSON *value, const Draw &draw)
    -> std::vector<std::string>;

// The names an entry of `patternProperties` is about.
auto pattern_name(const sourcemeta::core::JSON *entry) -> std::string;

// Words that take no article: "each one text", not "each one a text".
auto with_article(const std::string &words) -> std::string;

auto capitalised(const std::string &sentence) -> std::string;

// A value said briefly, naming a fixed value outright.
auto brief(const sourcemeta::core::JSON *value, const Draw &draw)
    -> std::string;

// A choice between one group of properties and a few plain values, such as a
// schema that may be an object or a boolean, is drawn as the group with the
// other types noted beside it.
auto as_group(const sourcemeta::core::JSON *value, Draw &draw)
    -> std::optional<Group>;

// What a branch of a condition, or anything pointed at, holds.
auto content_of(const sourcemeta::core::JSON *value, const Draw &draw)
    -> const sourcemeta::core::JSON *;

// Whether any form of a choice carries the author's words.
auto forms_say(const sourcemeta::core::JSON *value) -> bool;

// How one form of a choice is named.
auto form_text(const sourcemeta::core::JSON *option, const Draw &draw)
    -> std::string;

// The word for one or many, and with `counted` the number before it too.
auto plural(const std::size_t amount, const std::string &one,
            const std::string &many, const bool counted = false) -> std::string;

auto used(const sourcemeta::core::JSON *shape) -> std::size_t;

auto row_count(const Rows &rows) -> std::size_t;

// The rows a group of properties is drawn as: its named properties, the
// properties named by a pattern, the properties not otherwise named, and its
// conditions, in that order. Properties it inherits from a shape come
// separately, grouped by shape.
auto rows_of(const sourcemeta::core::JSON *value, const Draw &draw,
             const std::set<std::string> &seen = {}) -> Rows;

// A shape, by its name. As a type, it reads with an article.
auto link(Writer &out, const std::string &reference, const Draw &draw,
          const bool as_type = true) -> void;

auto value_cell(Writer &out, const sourcemeta::core::JSON *value, Draw &draw)
    -> void;

// A table of rows, or nothing when there are no rows.
auto table(Writer &out, const std::string &rows, const bool sub = false)
    -> void;

// Every row of a group, and the inherited ones behind a strip.
auto render_group(const sourcemeta::core::JSON *value, Draw &draw,
                  const std::optional<std::string> &under) -> std::string;

// A group made only of names the writer chooses is said on its row.
auto is_map(const sourcemeta::core::JSON *value) -> bool;

// Whether the properties under a map's chosen names have anything to draw.
auto map_has_inside(const sourcemeta::core::JSON *value, const Draw &draw)
    -> bool;

// What is drawn inside a row: the properties of a group, of each entry of a
// list, of each form of a choice, and the fixed slots of a list.
auto nested_of(Writer &out, const sourcemeta::core::JSON *value, Draw &draw,
               const std::string &name, const std::optional<std::string> &under)
    -> void;

} // namespace sourcemeta::blaze

#endif
