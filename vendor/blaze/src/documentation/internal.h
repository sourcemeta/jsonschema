#ifndef SOURCEMETA_BLAZE_DOCUMENTATION_INTERNAL_H_
#define SOURCEMETA_BLAZE_DOCUMENTATION_INTERNAL_H_

#include <sourcemeta/blaze/documentation_error.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

// What both halves of this module need to agree on: how old a dialect is, what
// a shape is, and what is being carried through a conversion. Reading one
// subschema lives in `documentation_value.cc`, and walking a whole schema in
// `documentation.cc`.

namespace sourcemeta::blaze {

enum class Rank : std::uint8_t {
  Draft3 = 3,
  Draft4 = 4,
  Draft6 = 6,
  Draft7 = 7,
  Draft2019 = 8,
  Draft2020 = 9
};

inline auto operator<(const Rank left, const Rank right) -> bool {
  return static_cast<std::uint8_t>(left) < static_cast<std::uint8_t>(right);
}

inline auto operator>=(const Rank left, const Rank right) -> bool {
  return !(left < right);
}

struct Shape {
  std::string identifier;
  sourcemeta::core::Pointer pointer;
  // How many places refer to it. The format's `usedAt` holds pointers into the
  // document rather than into the schema, so it is left out until the module
  // can state those exactly.
  std::size_t uses;
};

// A schema nested deeper than this is refused, so that a runaway schema gets
// an answer rather than exhausting the stack.
constexpr std::size_t MAXIMUM_DEPTH{64};

struct DocumentationContext {
  const sourcemeta::core::JSON &root;
  const sourcemeta::core::SchemaWalker &walker;
  const sourcemeta::core::SchemaResolver &resolver;
  const sourcemeta::core::SchemaFrame &frame;
  // The dialect of the subschema being described. A resource may declare a
  // dialect of its own, so this changes as the conversion moves through it.
  Rank rank;
  // The dialect of every subschema, by the place it sits in the schema.
  std::map<std::string, Rank> ranks;
  std::map<std::string, Shape> shapes;
  // Where each reference leads, by the place the `$ref` sits in the schema.
  std::map<std::string, sourcemeta::core::JSON::String> references;
  // The same, for a `$dynamicRef` or `$recursiveRef` whose target depends on
  // where it is used.
  std::map<std::string, sourcemeta::core::JSON::String> dynamic_references;
  // Every place that declares a dynamic anchor, by its name with the leading
  // `#`. A `$recursiveAnchor` has no name, so it is kept under an empty one.
  std::map<std::string, std::vector<sourcemeta::core::Pointer>> dynamic_anchors;
  // The keywords of each subschema, by the place it sits in the schema, that
  // belong to its dialect but to a vocabulary its meta-schema leaves out, so
  // they say nothing there.
  std::map<std::string, std::set<std::string>> ignored;
  std::size_t depth{0};
};

// The message is a fixed sentence. Whatever the sentence is about, such as
// the keyword at fault, travels beside it rather than inside it.
[[noreturn]] inline auto fail(const char *message,
                              const sourcemeta::core::Pointer &location,
                              std::string subject = "") -> void {
  throw sourcemeta::blaze::DocumentationError{message, location,
                                              std::move(subject)};
}

inline auto defines(const sourcemeta::core::JSON &subschema,
                    const std::string &keyword) -> bool {
  return subschema.is_object() && subschema.defines(keyword);
}

inline auto make_value(const std::string_view kind) -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};
  result.assign("kind", sourcemeta::core::JSON{std::string{kind}});
  return result;
}

inline auto bounds(const sourcemeta::core::JSON &subschema,
                   const std::string &minimum, const std::string &maximum)
    -> std::optional<sourcemeta::core::JSON> {
  const auto has_minimum{defines(subschema, minimum)};
  const auto has_maximum{defines(subschema, maximum)};
  if (!has_minimum && !has_maximum) {
    return std::nullopt;
  }

  auto result{sourcemeta::core::JSON::make_object()};
  if (has_minimum) {
    result.assign("min", subschema.at(minimum));
  }

  if (has_maximum) {
    result.assign("max", subschema.at(maximum));
  }

  return result;
}

inline auto copy_if_present(const sourcemeta::core::JSON &subschema,
                            const std::string &keyword,
                            sourcemeta::core::JSON &value,
                            const std::string &into) -> void {
  if (!defines(subschema, keyword)) {
    return;
  }

  // Text that says nothing is not there at all. A default or an example of an
  // empty string is a value, not text about the schema, and is kept.
  const auto &entry{subschema.at(keyword)};
  if ((keyword == "title" || keyword == "description") && entry.is_string() &&
      entry.to_string().empty()) {
    return;
  }

  value.assign(into, entry);
}

// Whether one value of an enumeration is accepted by the KEYWORDS beside it.
// Unknown means this module cannot tell, and then those KEYWORDS are stated as
// a rule of their own rather than being dropped.
enum class Verdict : std::uint8_t { Yes, No, Unknown };

inline auto also(sourcemeta::core::JSON &value, sourcemeta::core::JSON &&other)
    -> void {
  if (!value.defines("also")) {
    value.assign("also", sourcemeta::core::JSON::make_array());
  }

  value.at("also").push_back(std::move(other));
}

inline auto condition(sourcemeta::core::JSON &value,
                      sourcemeta::core::JSON &&entry) -> void {
  if (!value.defines("conditions")) {
    value.assign("conditions", sourcemeta::core::JSON::make_array());
  }

  value.at("conditions").push_back(std::move(entry));
}

// What the parts of a subschema that apply in place look at, as deep as they
// go: the properties they name, and the items they reach.
struct Reach {
  std::set<sourcemeta::core::JSON::String> names;
  std::set<sourcemeta::core::JSON::String> patterns;
  // Whether a part that always applies looks at every property.
  bool every_property{false};
  // Whether any part, even one that only sometimes applies, says what every
  // property it does not name holds, which names every property.
  bool names_every_property{false};
  // Whether a part that always applies looks at every item.
  bool every_item{false};
  // How many items at the start the parts that always apply look at.
  std::size_t prefix{0};
  // What the parts that always apply say some items contain. From 2020-12,
  // such an item counts as looked at.
  std::vector<sourcemeta::core::Pointer> contains;
  // Whether a part that only applies sometimes looks at items.
  bool items_sometimes{false};
};

// Defined in `documentation_reference.cc`.
// The dialect in effect at a place: the one of the subschema there, or else of
// the nearest one around it.
auto rank_at(const DocumentationContext &context,
             const sourcemeta::core::Pointer &pointer) -> Rank;

// Defined in `documentation_reference.cc`.
// Where a reference keyword of a subschema lands in this schema. Nothing means
// it leads to another document, kept at the address it gives.
auto landing_of(const DocumentationContext &context,
                const sourcemeta::core::Pointer &pointer,
                const std::string &keyword, std::string &destination)
    -> std::optional<sourcemeta::core::Pointer>;

// Defined in `documentation_reference.cc`.
// Where a `$dynamicRef` or `$recursiveRef` may land. Evaluation always starts
// at the root, so an anchor the root resource declares always wins. Otherwise
// the place depends on the path taken to get there, and each place it may be
// is one of the forms the value takes.
auto dynamic_landings(const DocumentationContext &context,
                      const sourcemeta::core::Pointer &pointer,
                      const std::string &keyword)
    -> std::vector<sourcemeta::core::Pointer>;

// Defined in `documentation_reference.cc`.
// What the reference keywords of a subschema lead to, one value for each.
auto references_of(DocumentationContext &context,
                   const sourcemeta::core::JSON &subschema,
                   const sourcemeta::core::Pointer &pointer)
    -> std::vector<sourcemeta::core::JSON>;

// Defined in `documentation_reach.cc`.
auto reach_of(const DocumentationContext &context,
              const sourcemeta::core::JSON &subschema,
              const sourcemeta::core::Pointer &pointer) -> Reach;

// Defined in `documentation_object.cc`.
auto object_facts(DocumentationContext &context, sourcemeta::core::JSON &value,
                  const sourcemeta::core::JSON &subschema,
                  const sourcemeta::core::Pointer &pointer) -> void;

// Defined in `documentation_facts.cc`.
auto accepted_by(const sourcemeta::core::JSON &entry,
                 const sourcemeta::core::JSON &subschema,
                 const std::vector<std::string> &kinds, const Rank rank)
    -> Verdict;

// Defined in `documentation_facts.cc`.
auto string_facts(DocumentationContext &context, sourcemeta::core::JSON &value,
                  const sourcemeta::core::JSON &subschema,
                  const sourcemeta::core::Pointer &pointer) -> void;

// Defined in `documentation_facts.cc`.
auto number_facts(sourcemeta::core::JSON &value,
                  const sourcemeta::core::JSON &subschema, const Rank rank)
    -> void;

// Defined in `documentation_facts.cc`.
auto array_facts(DocumentationContext &context, sourcemeta::core::JSON &value,
                 const sourcemeta::core::JSON &subschema,
                 const sourcemeta::core::Pointer &pointer) -> void;

// One subschema, described. Defined in `documentation_value.cc`.
auto describe_subschema(DocumentationContext &context,
                        const sourcemeta::core::JSON &subschema,
                        const sourcemeta::core::Pointer &pointer)
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::blaze

#endif
