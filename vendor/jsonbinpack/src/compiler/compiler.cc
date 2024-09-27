#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsonbinpack/numeric.h>

#include <sourcemeta/alterschema/engine.h>
#include <sourcemeta/alterschema/linter.h>

#include "encoding.h"

namespace sourcemeta::jsonbinpack {

auto canonicalize(sourcemeta::jsontoolkit::JSON &schema,
                  const sourcemeta::jsontoolkit::SchemaWalker &walker,
                  const sourcemeta::jsontoolkit::SchemaResolver &resolver,
                  const std::optional<std::string> &default_dialect) -> void {
  namespace alterschema = sourcemeta::alterschema;
  alterschema::Bundle canonicalizer;
  alterschema::add(canonicalizer, alterschema::LinterCategory::AntiPattern);
  alterschema::add(canonicalizer, alterschema::LinterCategory::Simplify);
  alterschema::add(canonicalizer, alterschema::LinterCategory::Desugar);
  alterschema::add(canonicalizer, alterschema::LinterCategory::Implicit);
  alterschema::add(canonicalizer, alterschema::LinterCategory::Superfluous);
  canonicalizer.apply(schema, walker, make_resolver(resolver),
                      sourcemeta::jsontoolkit::empty_pointer, default_dialect);
}

auto make_encoding(sourcemeta::alterschema::Transformer &document,
                   const std::string &encoding,
                   const sourcemeta::jsontoolkit::JSON &options) -> void {
  document.replace(sourcemeta::jsontoolkit::JSON::make_object());
  document.assign("$schema", sourcemeta::jsontoolkit::JSON{ENCODING_V1});
  document.assign("binpackEncoding", sourcemeta::jsontoolkit::JSON{encoding});
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

auto compile(sourcemeta::jsontoolkit::JSON &schema,
             const sourcemeta::jsontoolkit::SchemaWalker &walker,
             const sourcemeta::jsontoolkit::SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect) -> void {
  canonicalize(schema, walker, resolver, default_dialect);

  sourcemeta::alterschema::Bundle mapper;

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
               sourcemeta::jsontoolkit::empty_pointer, default_dialect);

  // The "any" encoding is always the last resort
  const auto dialect{sourcemeta::jsontoolkit::dialect(schema)};
  if (!dialect.has_value() || dialect.value() != ENCODING_V1) {
    sourcemeta::alterschema::Transformer transformer{schema};
    make_encoding(transformer, "ANY_PACKED_TYPE_TAG_BYTE_PREFIX",
                  sourcemeta::jsontoolkit::JSON::make_object());
  }
}

} // namespace sourcemeta::jsonbinpack
