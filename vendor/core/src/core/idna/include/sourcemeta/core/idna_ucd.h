#ifndef SOURCEMETA_CORE_IDNA_UCD_H_
#define SOURCEMETA_CORE_IDNA_UCD_H_

#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

/// @ingroup idna
/// The RFC 5892 derived property of a Unicode codepoint. See
/// https://www.rfc-editor.org/rfc/rfc5892 for the property's definition.
enum class IDNAProperty : std::uint8_t {
  PValid = 0,
  ContextJ = 1,
  ContextO = 2,
  Disallowed = 3,
  Unassigned = 4,
};

} // namespace sourcemeta::core

#endif
