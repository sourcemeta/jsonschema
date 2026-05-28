#ifndef SOURCEMETA_CORE_IDNA_UCD_H_
#define SOURCEMETA_CORE_IDNA_UCD_H_

#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

/// @ingroup idna
/// Each entry maps an `IDNAProperty` enum name to its RFC 5892 token.
#define SOURCEMETA_CORE_IDNA_PROPERTY_LIST(X)                                  \
  X(PValid, "PVALID")                                                          \
  X(ContextJ, "CONTEXTJ")                                                      \
  X(ContextO, "CONTEXTO")                                                      \
  X(Disallowed, "DISALLOWED")                                                  \
  X(Unassigned, "UNASSIGNED")

/// @ingroup idna
/// The RFC 5892 derived property of a Unicode codepoint. See
/// https://www.rfc-editor.org/rfc/rfc5892 for the property's definition.
enum class IDNAProperty : std::uint8_t {
#define SOURCEMETA_CORE_IDNA_ENUM_ENTRY(name, alias) name,
  SOURCEMETA_CORE_IDNA_PROPERTY_LIST(SOURCEMETA_CORE_IDNA_ENUM_ENTRY)
#undef SOURCEMETA_CORE_IDNA_ENUM_ENTRY
};

} // namespace sourcemeta::core

#endif
