#ifndef SOURCEMETA_CORE_OPENAPI_DIALECT_H_
#define SOURCEMETA_CORE_OPENAPI_DIALECT_H_

#include <sourcemeta/core/openapi.h>

#include <optional> // std::optional, std::nullopt

namespace sourcemeta::core {

// OpenAPI Specification 3.0.4, Schema Object, closes the keyword set of a
// Schema Object: "Additional keywords defined by the JSON Schema specification
// that are not mentioned here are strictly unsupported". No keyword naming a
// dialect is among the ones it tables, so a Schema Object of this revision has
// nothing to name one with, and every one of them is read under the single
// dialect the revision publishes. The same section says of each position that
// holds a subschema that the "Inline or referenced schema MUST be of a Schema
// Object and not a standard JSON Schema", which rules out a schema of another
// dialect reached from within one as much as one written there.
//
// The revisions from 3.1 onwards table a keyword for it instead, and Section
// 4.8.24.5 of 3.1.1 leaves the choice open: "Tooling MUST support the OAS
// dialect schema id, and MAY support additional values of `$schema`". So this
// is a question only the earlier revision answers with a refusal
//
// Which dialect is in force is asked of the vocabularies rather than of the
// URI naming it, as this revision has published more than one such URI and
// every release of it describes the same keywords, so one vocabulary answers
// for all of them. The set is exactly that one vocabulary, the base dialect
// being one that predates vocabularies altogether
inline auto openapi_is_3_0_vocabulary(const SchemaVocabularies &vocabularies)
    -> bool {
  return vocabularies.size() == 1 &&
         vocabularies.contains(SchemaVocabularies::Known::OPENAPI_3_0_BASE);
}

// Where a description reads a schema under a dialect its revision leaves it no
// way to name, if it does so anywhere. Only the places that a frame registers
// are asked, as a declaration that takes effect nowhere is one that settles
// nothing: a subschema that is no resource root of its own keeps whatever is
// in force around it, which JSON Schema 2020-12 Section 8.1.1 is what settles
//
// The places a schema answers to by an identifier are among those registered,
// and so are the places it answers to by where they sit, so one that is both
// is reached twice over and reported the same way each time
inline auto openapi_schema_of_another_dialect(const OpenAPIVersion version,
                                              const SchemaFrame &schemas,
                                              const SchemaFrame::Paths &paths,
                                              const SchemaResolver &resolver)
    -> std::optional<Pointer> {
  if (version != OpenAPIVersion::OPENAPI_3_0) {
    return std::nullopt;
  }

  std::optional<Pointer> result;
  schemas.for_each_location([&schemas, &paths, &resolver,
                             &result](const auto, const auto &,
                                      const auto &location) -> void {
    if (result.has_value() ||
        openapi_is_3_0_vocabulary(schemas.vocabularies(location, resolver))) {
      return;
    }

    // A frame of a description is handed each Schema Object position to frame
    // rather than the document as a whole, so what it holds besides those is
    // of no concern here
    for (const auto &path : paths) {
      if (location.pointer.starts_with(path)) {
        result = to_pointer(location.pointer);
        return;
      }
    }
  });

  return result;
}

} // namespace sourcemeta::core

#endif
