#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_REFERENCE_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_REFERENCE_H_

#include <cstdint> // std::uint8_t

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonschema
/// The reference type
enum class ReferenceType : std::uint8_t { Static, Dynamic };

} // namespace sourcemeta::jsontoolkit

#endif
