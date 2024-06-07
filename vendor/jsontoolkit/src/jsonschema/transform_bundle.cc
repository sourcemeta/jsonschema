#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error

// For built-in rules
#include <algorithm> // std::any_of
#include <iterator>  // std::cbegin, std::cend
namespace sourcemeta::jsontoolkit {
template <typename T>
auto contains_any(const T &container, const T &values) -> bool {
  return std::any_of(
      std::cbegin(container), std::cend(container),
      [&values](const auto &element) { return values.contains(element); });
}

// Modernize
#include "rules/enum_to_const.h"
// AntiPattern
#include "rules/const_with_type.h"
#include "rules/enum_with_type.h"
// Simplify
#include "rules/single_type_array.h"
// Redundant
#include "rules/additional_properties_default.h"
#include "rules/content_media_type_without_encoding.h"
#include "rules/content_schema_default.h"
#include "rules/content_schema_without_media_type.h"
#include "rules/else_without_if.h"
#include "rules/items_array_default.h"
#include "rules/items_schema_default.h"
#include "rules/max_contains_without_contains.h"
#include "rules/min_contains_without_contains.h"
#include "rules/then_without_if.h"
#include "rules/unevaluated_items_default.h"
#include "rules/unevaluated_properties_default.h"
} // namespace sourcemeta::jsontoolkit

auto sourcemeta::jsontoolkit::SchemaTransformBundle::add(
    const sourcemeta::jsontoolkit::SchemaTransformBundle::Category category)
    -> void {
  switch (category) {
    case sourcemeta::jsontoolkit::SchemaTransformBundle::Category::Modernize:
      this->template add<EnumToConst>();
      break;
    case sourcemeta::jsontoolkit::SchemaTransformBundle::Category::AntiPattern:
      this->template add<EnumWithType>();
      this->template add<ConstWithType>();
      break;
    case sourcemeta::jsontoolkit::SchemaTransformBundle::Category::Simplify:
      this->template add<SingleTypeArray>();
      break;
    case sourcemeta::jsontoolkit::SchemaTransformBundle::Category::Redundant:
      this->template add<AdditionalPropertiesDefault>();
      this->template add<ContentMediaTypeWithoutEncoding>();
      this->template add<ContentSchemaDefault>();
      this->template add<ContentSchemaWithoutMediaType>();
      this->template add<ElseWithoutIf>();
      this->template add<ItemsArrayDefault>();
      this->template add<ItemsSchemaDefault>();
      this->template add<MaxContainsWithoutContains>();
      this->template add<MinContainsWithoutContains>();
      this->template add<ThenWithoutIf>();
      this->template add<UnevaluatedItemsDefault>();
      this->template add<UnevaluatedPropertiesDefault>();
      break;
    default:
      // We should never get here
      assert(false);
      break;
  }
}

auto sourcemeta::jsontoolkit::SchemaTransformBundle::apply(
    sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const sourcemeta::jsontoolkit::Pointer &pointer,
    const std::optional<std::string> &default_dialect) const -> void {
  // There is no point in applying an empty bundle
  assert(!this->rules.empty());

  auto &current{sourcemeta::jsontoolkit::get(schema, pointer)};
  const std::optional<std::string> root_dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(current, root_dialect)};

  // (1) Transform the current schema object
  // Avoid recursion to not blow up the stack even on highly complex schemas
  std::set<std::string> processed_rules;
  while (true) {
    auto matches{processed_rules.size()};
    for (const auto &[name, rule] : this->rules) {
      // TODO: Process traces to fixup references
      const auto traces{rule->apply(current, pointer, resolver, dialect)};
      if (!traces.empty()) {
        if (processed_rules.contains(name)) {
          std::ostringstream error;
          error << "Rules must only be processed once: " << name;
          throw std::runtime_error(error.str());
        }

        processed_rules.insert(name);
      }
    }

    if (matches < processed_rules.size()) {
      continue;
    }

    break;
  }

  // (2) Transform its sub-schemas
  for (const auto &entry : sourcemeta::jsontoolkit::SchemaIteratorFlat{
           current, walker, resolver, dialect}) {
    apply(schema, walker, resolver, pointer.concat(entry.pointer), dialect);
  }
}

auto sourcemeta::jsontoolkit::SchemaTransformBundle::check(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const sourcemeta::jsontoolkit::SchemaTransformBundle::CheckCallback
        &callback,
    const sourcemeta::jsontoolkit::Pointer &pointer,
    const std::optional<std::string> &default_dialect) const -> bool {
  const auto &current{sourcemeta::jsontoolkit::get(schema, pointer)};
  const std::optional<std::string> root_dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(current, root_dialect)};

  bool result{true};
  for (const auto &entry : sourcemeta::jsontoolkit::SchemaIterator{
           current, walker, resolver, dialect}) {
    const auto current_pointer{pointer.concat(entry.pointer)};
    for (const auto &[name, rule] : this->rules) {
      if (rule->check(sourcemeta::jsontoolkit::get(current, entry.pointer),
                      current_pointer, resolver, dialect)) {
        result = false;
        callback(current_pointer, name, rule->message());
      }
    }
  }

  return result;
}
