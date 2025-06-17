#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsonbinpack/numeric.h>

#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/jsonpointer.h>

#include "encoding.h"

static auto transformer_callback_noop(const sourcemeta::core::Pointer &,
                                      const std::string_view,
                                      const std::string_view,
                                      const std::string_view) -> void {
  // This callback should never be called, as all the transformation rules
  // we define in this project can indeed be transformed
  assert(false);
}

namespace sourcemeta::jsonbinpack {

auto canonicalize(sourcemeta::core::JSON &schema,
                  const sourcemeta::core::SchemaWalker &walker,
                  const sourcemeta::core::SchemaResolver &resolver,
                  const std::optional<std::string> &default_dialect) -> void {
  sourcemeta::core::SchemaTransformer canonicalizer;
  sourcemeta::core::add(canonicalizer,
                        sourcemeta::core::AlterSchemaMode::StaticAnalysis);
  canonicalizer.apply(schema, walker, make_resolver(resolver),
                      transformer_callback_noop, default_dialect);
}

auto make_encoding(sourcemeta::core::JSON &document,
                   const std::string &encoding,
                   const sourcemeta::core::JSON &options) -> void {
  document.into_object();
  document.assign("$schema", sourcemeta::core::JSON{ENCODING_V1});
  document.assign("binpackEncoding", sourcemeta::core::JSON{encoding});
  document.assign("binpackOptions", options);
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

auto compile(sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect) -> void {
  canonicalize(schema, walker, resolver, default_dialect);

  sourcemeta::core::SchemaTransformer mapper;

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

  mapper.apply(schema, walker, make_resolver(resolver),
               transformer_callback_noop, default_dialect);

  // The "any" encoding is always the last resort
  const auto dialect{sourcemeta::core::dialect(schema)};
  if (!dialect.has_value() || dialect.value() != ENCODING_V1) {
    make_encoding(schema, "ANY_PACKED_TYPE_TAG_BYTE_PREFIX",
                  sourcemeta::core::JSON::make_object());
  }
}

} // namespace sourcemeta::jsonbinpack
