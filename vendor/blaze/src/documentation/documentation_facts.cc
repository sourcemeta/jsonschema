#include "internal.h"

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/regex.h>

#include <algorithm>   // std::sort, std::any_of, std::find, std::max
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int64_t, std::uint8_t, std::uint32_t
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair
#include <vector>      // std::vector

// What text, numbers and lists may be, and whether a fixed value passes the
// rules beside it.

namespace {

using sourcemeta::blaze::defines;
using sourcemeta::blaze::Rank;
using sourcemeta::core::JSON;
using sourcemeta::core::Pointer;

auto kind_of_value(const JSON &entry, const Rank rank) -> std::string {
  if (entry.is_string()) {
    return "string";
  }

  if (entry.is_integer()) {
    return "integer";
  }

  // From Draft 6, a number with a zero fraction is an integer.
  if (entry.is_real()) {
    return rank >= Rank::Draft6 && entry.is_integral() ? "integer" : "number";
  }

  // A number too large for the other kinds, which may still be whole.
  if (entry.is_decimal()) {
    return entry.to_decimal().is_integral() ? "integer" : "number";
  }

  if (entry.is_boolean()) {
    return "boolean";
  }

  if (entry.is_null()) {
    return "null";
  }

  if (entry.is_array()) {
    return "array";
  }

  return "object";
}

// Where a number sits beside a limit: below it, on it, or above it. Numbers
// are compared exactly, whatever kind of number each one is. Going through
// `double` would make distinct integers past the range it holds exactly look
// equal, and a value the schema rejects would be kept as one it accepts.
auto side_of(const JSON &number, const JSON &limit) -> int {
  if (number < limit) {
    return -1;
  }

  return limit < number ? 1 : 0;
}

auto numeric_bound(const JSON &subschema, const std::string &keyword)
    -> const JSON * {
  if (!defines(subschema, keyword)) {
    return nullptr;
  }

  const auto &bound{subschema.at(keyword)};
  return bound.is_number() ? &bound : nullptr;
}

// Before Draft 6 an exclusive bound is a flag beside the bound itself, rather
// than a number of its own.
auto exclusive_flag(const JSON &subschema, const Rank rank,
                    const std::string &keyword) -> bool {
  return rank < Rank::Draft6 && defines(subschema, keyword) &&
         subschema.at(keyword).is_boolean() &&
         subschema.at(keyword).to_boolean();
}

} // namespace

namespace sourcemeta::blaze {

auto accepted_by(const JSON &entry, const JSON &subschema,
                 const std::vector<std::string> &kinds, const Rank rank)
    -> Verdict {
  const auto kind{kind_of_value(entry, rank)};
  if (!kinds.empty()) {
    auto matches{false};
    for (const auto &wanted : kinds) {
      if (wanted == kind || (wanted == "number" && kind == "integer")) {
        matches = true;
      }
    }

    if (!matches) {
      return Verdict::No;
    }
  }

  if (entry.is_string()) {
    // `minLength` and `maxLength` count characters, not bytes, and
    // `JSON::size` is the logical length the specification means.
    const JSON length{static_cast<std::int64_t>(entry.size())};
    if (defines(subschema, "minLength") &&
        subschema.at("minLength").is_number() &&
        length < subschema.at("minLength")) {
      return Verdict::No;
    }

    if (defines(subschema, "maxLength") &&
        subschema.at("maxLength").is_number() &&
        subschema.at("maxLength") < length) {
      return Verdict::No;
    }

    // A pattern needs a regular expression engine to decide, so it is left to
    // the rule beside the values.
    if (defines(subschema, "pattern")) {
      return Verdict::Unknown;
    }
  }

  if (entry.is_number()) {
    const auto *low{numeric_bound(subschema, "minimum")};
    if (low != nullptr) {
      const auto where{side_of(entry, *low)};
      if (where < 0 ||
          (where == 0 && exclusive_flag(subschema, rank, "exclusiveMinimum"))) {
        return Verdict::No;
      }
    }

    const auto *high{numeric_bound(subschema, "maximum")};
    if (high != nullptr) {
      const auto where{side_of(entry, *high)};
      if (where > 0 ||
          (where == 0 && exclusive_flag(subschema, rank, "exclusiveMaximum"))) {
        return Verdict::No;
      }
    }

    if (rank >= Rank::Draft6) {
      const auto *above{numeric_bound(subschema, "exclusiveMinimum")};
      if (above != nullptr && side_of(entry, *above) <= 0) {
        return Verdict::No;
      }

      const auto *below{numeric_bound(subschema, "exclusiveMaximum")};
      if (below != nullptr && side_of(entry, *below) >= 0) {
        return Verdict::No;
      }
    }
  }

  // Anything else beside the values, such as an object with properties of its
  // own, a reference, or a combination, is left to the rule beside them.
  static const std::vector<std::string> DECIDED{"type",
                                                "enum",
                                                "const",
                                                "minLength",
                                                "maxLength",
                                                "pattern",
                                                "minimum",
                                                "maximum",
                                                "exclusiveMinimum",
                                                "exclusiveMaximum",
                                                "title",
                                                "description",
                                                "default",
                                                "examples",
                                                "$comment",
                                                "deprecated",
                                                "$schema",
                                                "$id",
                                                "format"};
  if (subschema.is_object()) {
    for (const auto &entry_pair : subschema.as_object()) {
      if (std::find(DECIDED.cbegin(), DECIDED.cend(), entry_pair.first) ==
          DECIDED.cend()) {
        return Verdict::Unknown;
      }
    }
  }

  return Verdict::Yes;
}

auto string_facts(DocumentationContext &context, JSON &value,
                  const JSON &subschema, const Pointer &pointer) -> void {
  copy_if_present(subschema, "format", value, "format");
  if (defines(subschema, "pattern")) {
    const auto &pattern{subschema.at("pattern")};
    assert(pattern.is_string());

    value.assign("pattern", pattern);
  }

  const auto length{bounds(subschema, "minLength", "maxLength")};
  if (length.has_value()) {
    value.assign("length", JSON{length.value()});
  }

  if (context.rank >= Rank::Draft7 && (defines(subschema, "contentMediaType") ||
                                       defines(subschema, "contentEncoding") ||
                                       defines(subschema, "contentSchema"))) {
    auto content{JSON::make_object()};
    copy_if_present(subschema, "contentMediaType", content, "mediaType");
    copy_if_present(subschema, "contentEncoding", content, "encoding");
    if (defines(subschema, "contentSchema") &&
        context.rank >= Rank::Draft2019) {
      content.assign("value",
                     describe_subschema(context, subschema.at("contentSchema"),
                                        pointer.concat("contentSchema")));
    }

    if (!content.empty()) {
      value.assign("content", std::move(content));
    }
  }
}

auto number_facts(JSON &value, const JSON &subschema, const Rank rank) -> void {
  copy_if_present(subschema, "format", value, "format");
  auto range{JSON::make_object()};
  if (defines(subschema, "minimum")) {
    range.assign("min", subschema.at("minimum"));
  }

  if (defines(subschema, "maximum")) {
    range.assign("max", subschema.at("maximum"));
  }

  // In Draft 4, the exclusive KEYWORDS are flags on the bounds; from Draft 6
  // they are bounds of their own.
  if (defines(subschema, "exclusiveMinimum")) {
    const auto &exclusive{subschema.at("exclusiveMinimum")};
    if (rank < Rank::Draft6) {
      assert(exclusive.is_boolean());

      assert(!exclusive.to_boolean() || range.defines("min"));

      if (exclusive.to_boolean()) {
        range.assign("minExclusive", JSON{true});
      }
    } else if (!range.defines("min")) {
      range.assign("min", exclusive);
      range.assign("minExclusive", JSON{true});
    } else {
      // Both bounds hold, so the tighter one is the rule.
      const auto &inclusive{range.at("min")};
      if (inclusive > exclusive) {
        range.erase(JSON::String{"minExclusive"});
      } else {
        range.assign("min", exclusive);
        range.assign("minExclusive", JSON{true});
      }
    }
  }

  if (defines(subschema, "exclusiveMaximum")) {
    const auto &exclusive{subschema.at("exclusiveMaximum")};
    if (rank < Rank::Draft6) {
      assert(exclusive.is_boolean());

      assert(!exclusive.to_boolean() || range.defines("max"));

      if (exclusive.to_boolean()) {
        range.assign("maxExclusive", JSON{true});
      }
    } else if (!range.defines("max")) {
      range.assign("max", exclusive);
      range.assign("maxExclusive", JSON{true});
    } else {
      const auto &inclusive{range.at("max")};
      if (inclusive < exclusive) {
        range.erase(JSON::String{"maxExclusive"});
      } else {
        range.assign("max", exclusive);
        range.assign("maxExclusive", JSON{true});
      }
    }
  }

  if (!range.empty()) {
    value.assign("range", std::move(range));
  }

  // Draft 3 calls the same rule `divisibleBy`.
  const auto *const step_keyword{rank == Rank::Draft3 ? "divisibleBy"
                                                      : "multipleOf"};
  if (defines(subschema, step_keyword)) {
    const auto &step{subschema.at(step_keyword)};
    assert(step.is_number() && JSON{static_cast<std::int64_t>(0)} < step);

    value.assign("multipleOf", step);
  }
}

auto array_facts(DocumentationContext &context, JSON &value,
                 const JSON &subschema, const Pointer &pointer) -> void {
  if (defines(subschema, "prefixItems") && context.rank >= Rank::Draft2020) {
    const auto &slots{subschema.at("prefixItems")};
    assert(slots.is_array());

    auto entries{JSON::make_array()};
    for (std::size_t index = 0; index < slots.array_size(); index++) {
      entries.push_back(
          describe_subschema(context, slots.at(index),
                             pointer.concat("prefixItems").concat(index)));
    }

    value.assign("slots", std::move(entries));
  }

  // Whether this subschema itself says what every entry after the slots holds.
  auto rest_said{false};
  if (defines(subschema, "items")) {
    const auto &items{subschema.at("items")};
    if (items.is_array()) {
      assert(context.rank < Rank::Draft2020);

      auto entries{JSON::make_array()};
      for (std::size_t index = 0; index < items.array_size(); index++) {
        entries.push_back(describe_subschema(
            context, items.at(index), pointer.concat("items").concat(index)));
      }

      value.assign("slots", std::move(entries));
      if (defines(subschema, "additionalItems")) {
        value.assign(
            "item", describe_subschema(context, subschema.at("additionalItems"),
                                       pointer.concat("additionalItems")));
        rest_said = true;
      }
    } else {
      value.assign("item",
                   describe_subschema(context, items, pointer.concat("items")));
      rest_said = true;
    }
  }

  // An entry counts as looked at when this subschema, or a part of it that
  // applies in place, says what it holds.
  if (defines(subschema, "unevaluatedItems") &&
      context.rank >= Rank::Draft2019 && !rest_said) {
    const auto reach{reach_of(context, subschema, pointer)};
    if (reach.items_sometimes) {
      fail("This keyword beside a part that only sometimes applies, and looks "
           "at entries, depends on which part matched, which the format "
           "cannot state exactly",
           pointer, "unevaluatedItems");
    }

    const auto &unevaluated{subschema.at("unevaluatedItems")};
    if (!reach.every_item &&
        !(unevaluated.is_boolean() && unevaluated.to_boolean())) {
      // The entries at the start that some part looks at are not held to it,
      // so they become slots that say nothing of their own.
      if (!value.defines("slots") && reach.prefix > 0) {
        value.assign("slots", JSON::make_array());
      }

      while (value.defines("slots") &&
             value.at("slots").array_size() < reach.prefix) {
        value.at("slots").push_back(make_value("any"));
      }

      std::vector<JSON> options;
      auto rest{describe_subschema(context, unevaluated,
                                   pointer.concat("unevaluatedItems"))};
      if (rest.at("kind").to_string() != "never" || rest.size() > 1) {
        options.push_back(std::move(rest));
      }

      // From 2020-12, an entry that matches `contains` counts as looked at.
      std::vector<Pointer> contains{reach.contains};
      if (context.rank >= Rank::Draft2020 && defines(subschema, "contains")) {
        contains.push_back(pointer.concat("contains"));
      }

      for (const auto &place : contains) {
        options.push_back(describe_subschema(
            context, sourcemeta::core::get(context.root, place), place));
      }

      if (options.empty()) {
        value.assign("item", make_value("never"));
      } else if (options.size() == 1) {
        value.assign("item", std::move(options.front()));
      } else {
        auto entries{JSON::make_array()};
        for (auto &option : options) {
          entries.push_back(std::move(option));
        }

        auto choice{make_value("choice")};
        choice.assign("options", std::move(entries));
        choice.assign("overlap", JSON{true});
        value.assign("item", std::move(choice));
      }
    }
  }

  const auto length{bounds(subschema, "minItems", "maxItems")};
  if (length.has_value()) {
    value.assign("length", JSON{length.value()});
  }

  if (defines(subschema, "uniqueItems") &&
      subschema.at("uniqueItems").is_boolean() &&
      subschema.at("uniqueItems").to_boolean()) {
    value.assign("unique", JSON{true});
  }

  if (defines(subschema, "contains") && context.rank >= Rank::Draft6) {
    auto contains{JSON::make_object()};
    contains.assign("value",
                    describe_subschema(context, subschema.at("contains"),
                                       pointer.concat("contains")));
    if (defines(subschema, "minContains") && context.rank >= Rank::Draft2019) {
      contains.assign("min", subschema.at("minContains"));
    } else {
      contains.assign("min", JSON{static_cast<std::int64_t>(1)});
    }

    if (defines(subschema, "maxContains") && context.rank >= Rank::Draft2019) {
      contains.assign("max", subschema.at("maxContains"));
    }

    value.assign("contains", std::move(contains));
  }
}

} // namespace sourcemeta::blaze
