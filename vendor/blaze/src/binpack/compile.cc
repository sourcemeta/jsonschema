#include <sourcemeta/blaze/binpack.h>

#include <sourcemeta/blaze/alterschema.h>
#include <sourcemeta/blaze/canonicalizer.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/numeric.h>

#include <cassert>     // assert
#include <string>      // std::string
#include <string_view> // std::string_view
#include <type_traits> // std::true_type

namespace {

constexpr auto ENCODING_V1{"tag:sourcemeta.com,2024:jsonbinpack/encoding/v1"};

auto make_encoding_resolver(const sourcemeta::core::SchemaResolver &fallback)
    -> auto {
  return [&fallback](std::string_view identifier)
             -> sourcemeta::core::SchemaResolverResult {
    if (identifier == ENCODING_V1) {
      static const auto SCHEMA{sourcemeta::core::parse_json(R"JSON({
        "$id": "tag:sourcemeta.com,2024:jsonbinpack/encoding/v1",
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$vocabulary": {
          "https://json-schema.org/draft/2020-12/vocab/core": true,
          "tag:sourcemeta.com,2024:jsonbinpack/encoding/v1": true
        }
      })JSON")};
      return SCHEMA;
    }

    return fallback(identifier);
  };
}

auto make_encoding(sourcemeta::core::JSON &document,
                   const std::string &encoding,
                   const sourcemeta::core::JSON &options) -> void {
  document.into_object();
  document.assign("$schema", sourcemeta::core::JSON{ENCODING_V1});
  document.assign("binpackEncoding", sourcemeta::core::JSON{encoding});
  document.assign("binpackOptions", options);
}

auto transformer_callback_noop(
    [[maybe_unused]] const sourcemeta::core::Pointer &pointer,
    [[maybe_unused]] const std::string_view name,
    [[maybe_unused]] const std::string_view message,
    [[maybe_unused]] const sourcemeta::blaze::SchemaTransformRule::Result
        &result,
    [[maybe_unused]] const bool applied) -> void {
  assert(applied);
}

} // namespace

namespace sourcemeta::blaze {

auto binpack_canonicalize(sourcemeta::core::JSON &schema,
                          const sourcemeta::core::SchemaWalker &walker,
                          const sourcemeta::core::SchemaResolver &resolver,
                          const std::string_view default_dialect) -> void {
  sourcemeta::blaze::canonicalize(
      schema, walker, make_encoding_resolver(resolver), default_dialect);
}

#include "mapper/enum_8_bit.h"
#include "mapper/enum_8_bit_top_level.h"
#include "mapper/enum_arbitrary.h"
#include "mapper/enum_singleton.h"
#include "mapper/integer_bounded_8_bit.h"
#include "mapper/integer_bounded_greater_than_8_bit.h"
#include "mapper/integer_bounded_multiplier_8_bit.h"
#include "mapper/integer_bounded_multiplier_greater_than_8_bit.h"
#include "mapper/integer_lower_bound.h"
#include "mapper/integer_lower_bound_multiplier.h"
#include "mapper/integer_unbound.h"
#include "mapper/integer_unbound_multiplier.h"
#include "mapper/integer_upper_bound.h"
#include "mapper/integer_upper_bound_multiplier.h"
#include "mapper/number_arbitrary.h"

auto binpack_compile(sourcemeta::core::JSON &schema,
                     const sourcemeta::core::SchemaWalker &walker,
                     const sourcemeta::core::SchemaResolver &resolver,
                     const std::string_view default_dialect) -> void {
  sourcemeta::blaze::binpack_canonicalize(schema, walker, resolver,
                                          default_dialect);

  sourcemeta::blaze::SchemaTransformer mapper;

  // Enums
  mapper.add<Enum8Bit>();
  mapper.add<Enum8BitTopLevel>();
  mapper.add<EnumArbitrary>();
  mapper.add<EnumSingleton>();

  // Integers
  mapper.add<IntegerBounded8Bit>();
  mapper.add<IntegerBoundedMultiplier8Bit>();
  mapper.add<IntegerBoundedGreaterThan8Bit>();
  mapper.add<IntegerBoundedMultiplierGreaterThan8Bit>();
  mapper.add<IntegerLowerBound>();
  mapper.add<IntegerLowerBoundMultiplier>();
  mapper.add<IntegerUpperBound>();
  mapper.add<IntegerUpperBoundMultiplier>();
  mapper.add<IntegerUnbound>();
  mapper.add<IntegerUnboundMultiplier>();

  // Numbers
  mapper.add<NumberArbitrary>();

  [[maybe_unused]] const auto mapper_result =
      mapper.apply(schema, walker, make_encoding_resolver(resolver),
                   transformer_callback_noop, default_dialect);
  assert(mapper_result.first);

  // The "any" encoding is always the last resort
  const auto *declared{schema.is_object() ? schema.try_at("$schema") : nullptr};
  const auto dialect{declared != nullptr && declared->is_string()
                         ? declared->to_string()
                         : std::string_view{}};
  if (dialect.empty() || dialect != ENCODING_V1) {
    make_encoding(schema, "ANY_PACKED_TYPE_TAG_BYTE_PREFIX",
                  sourcemeta::core::JSON::make_object());
  }
}

} // namespace sourcemeta::blaze
