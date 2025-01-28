#ifndef SOURCEMETA_CORE_JSONSCHEMA_REFERENCE_H_
#define SOURCEMETA_CORE_JSONSCHEMA_REFERENCE_H_

#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

/// @ingroup jsonschema
/// The reference type
enum class ReferenceType : std::uint8_t { Static, Dynamic };

} // namespace sourcemeta::core

#endif
