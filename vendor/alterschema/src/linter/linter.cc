#include <sourcemeta/alterschema/linter.h>

#include <cassert> // assert

// For built-in rules
#include <algorithm> // std::any_of
#include <iterator>  // std::cbegin, std::cend
namespace sourcemeta::alterschema {
template <typename T>
auto contains_any(const T &container, const T &values) -> bool {
  return std::any_of(
      std::cbegin(container), std::cend(container),
      [&values](const auto &element) { return values.contains(element); });
}

// Modernize
#include "modernize/enum_to_const.h"
// AntiPattern
#include "antipattern/const_with_type.h"
#include "antipattern/enum_with_type.h"
// Simplify
#include "simplify/single_type_array.h"
// Redundant
#include "redundant/additional_properties_default.h"
#include "redundant/content_media_type_without_encoding.h"
#include "redundant/content_schema_default.h"
#include "redundant/content_schema_without_media_type.h"
#include "redundant/else_without_if.h"
#include "redundant/items_array_default.h"
#include "redundant/items_schema_default.h"
#include "redundant/max_contains_without_contains.h"
#include "redundant/min_contains_without_contains.h"
#include "redundant/then_without_if.h"
#include "redundant/unevaluated_items_default.h"
#include "redundant/unevaluated_properties_default.h"
} // namespace sourcemeta::alterschema

namespace sourcemeta::alterschema {

auto add(Bundle &bundle, const LinterCategory category) -> void {
  switch (category) {
    case LinterCategory::Modernize:
      bundle.add<EnumToConst>();
      break;
    case LinterCategory::AntiPattern:
      bundle.add<EnumWithType>();
      bundle.add<ConstWithType>();
      break;
    case LinterCategory::Simplify:
      bundle.add<SingleTypeArray>();
      break;
    case LinterCategory::Redundant:
      bundle.add<AdditionalPropertiesDefault>();
      bundle.add<ContentMediaTypeWithoutEncoding>();
      bundle.add<ContentSchemaDefault>();
      bundle.add<ContentSchemaWithoutMediaType>();
      bundle.add<ElseWithoutIf>();
      bundle.add<ItemsArrayDefault>();
      bundle.add<ItemsSchemaDefault>();
      bundle.add<MaxContainsWithoutContains>();
      bundle.add<MinContainsWithoutContains>();
      bundle.add<ThenWithoutIf>();
      bundle.add<UnevaluatedItemsDefault>();
      bundle.add<UnevaluatedPropertiesDefault>();
      break;
    default:
      // We should never get here
      assert(false);
      break;
  }
}

} // namespace sourcemeta::alterschema
