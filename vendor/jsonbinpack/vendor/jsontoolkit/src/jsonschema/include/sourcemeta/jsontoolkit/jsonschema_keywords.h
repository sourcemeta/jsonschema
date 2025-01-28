#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_KEYWORDS_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_KEYWORDS_H_

#include <cstdint> // std::uint8_t

namespace sourcemeta::jsontoolkit {

#if defined(__GNUC__)
#pragma GCC diagnostic push
// For some strange reason, GCC on Debian 11 believes that a member of
// an enum class (which is namespaced by definition), can shadow an
// alias defined even on a different namespace.
#pragma GCC diagnostic ignored "-Wshadow"
#endif
/// @ingroup jsonschema
/// Determines the type of a JSON Schema keyword
enum class KeywordType : std::uint8_t {
  /// The JSON Schema keyword is unknown
  Unknown,
  /// The JSON Schema keyword is a non-applicator assertion
  Assertion,
  /// The JSON Schema keyword is a non-applicator annotation
  Annotation,
  /// The JSON Schema keyword is a reference
  Reference,
  /// The JSON Schema keyword is known but doesn't match any other type
  Other,
  /// The JSON Schema keyword is considered to be a comment without any
  /// additional meaning
  Comment,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument
  ApplicatorValue,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument but its evaluation follows
  /// special rules
  ApplicatorValueOther,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument without affecting the
  /// instance location
  ApplicatorValueInPlace,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an array of potentially JSON Schema definitions
  /// as an argument
  ApplicatorElements,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an array of potentially JSON Schema definitions
  /// as an argument without affecting the instance location
  ApplicatorElementsInPlace,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an array of potentially JSON Schema definitions
  /// as an argument without affecting the instance location and that can be
  /// statically inlined
  ApplicatorElementsInline,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an object as argument, whose values are potentially
  /// JSON Schema definitions
  ApplicatorMembers,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an object as argument, whose values are potentially
  /// JSON Schema definitions without affecting the instance location
  ApplicatorMembersInPlace,
  /// The JSON Schema keyword is an applicator that may take a JSON Schema
  /// definition or an array of potentially JSON Schema definitions
  /// as an argument
  ApplicatorValueOrElements,
  /// The JSON Schema keyword is an applicator that may take a JSON Schema
  /// definition or an array of potentially JSON Schema definitions
  /// as an argument without affecting the instance location
  ApplicatorValueOrElementsInPlace,
  /// The JSON Schema keyword is an applicator that may take an array of
  /// potentially JSON Schema definitions or an object whose values are
  /// potentially JSON Schema definitions as an argument
  ApplicatorElementsOrMembers,
  /// The JSON Schema keyword is a reserved location that potentially
  /// takes an object as argument, whose values are potentially
  /// JSON Schema definitions
  LocationMembers,
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} // namespace sourcemeta::jsontoolkit

#endif
