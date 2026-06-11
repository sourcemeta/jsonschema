#ifndef SOURCEMETA_CORE_IDNA_H_
#define SOURCEMETA_CORE_IDNA_H_

#ifndef SOURCEMETA_CORE_IDNA_EXPORT
#include <sourcemeta/core/idna_export.h>
#endif

#include <sourcemeta/core/idna_ucd.h>

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string>      // std::u32string
#include <string_view> // std::string_view, std::u32string_view

/// @defgroup idna IDNA
/// @brief Internationalized Domain Names in Applications (IDNA2008) utilities.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/idna.h>
/// ```

namespace sourcemeta::core {

/// @ingroup idna
/// The RFC 5890 §2.3.2 classification of a domain name label.
enum class IDNALabelKind : std::uint8_t {
  /// A pure ASCII label that is not an A-label. The IDNA specification
  /// does not validate such labels.
  Ascii = 0,
  /// An RFC 5890 §2.3.2.1 A-label (ACE form), validated per RFC 5891 §4.
  ALabel = 1,
  /// An RFC 5890 §2.3.2.2 U-label (Unicode form), validated per RFC 5891 §4.
  ULabel = 2,
};

/// @ingroup idna
/// Classify `label` as an Ascii / A-label / U-label per RFC 5890 §2.3.2,
/// validate the A-label and U-label cases per RFC 5891 §4, and write the
/// U-label codepoint form to `decoded`. Detection of the ACE prefix "xn--"
/// is case-insensitive per RFC 5890 §2.3.2.1. For example:
///
/// ```cpp
/// #include <sourcemeta/core/idna.h>
/// #include <cassert>
///
/// std::u32string decoded;
/// assert(sourcemeta::core::idna_classify_label(U"\u00DF\u03C2", decoded) ==
///        sourcemeta::core::IDNALabelKind::ULabel);
/// assert(sourcemeta::core::idna_classify_label(U"xn--mnchen-3ya", decoded) ==
///        sourcemeta::core::IDNALabelKind::ALabel);
/// assert(sourcemeta::core::idna_classify_label(U"example", decoded) ==
///        sourcemeta::core::IDNALabelKind::Ascii);
/// ```
SOURCEMETA_CORE_IDNA_EXPORT
auto idna_classify_label(const std::u32string_view label,
                         std::u32string &decoded)
    -> std::optional<IDNALabelKind>;

/// @ingroup idna
/// Return the RFC 5892 derived property of a Unicode codepoint. See
/// https://www.rfc-editor.org/rfc/rfc5892 for the property's definition.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/idna.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::idna_property(U'a') ==
///        sourcemeta::core::IDNAProperty::PValid);
/// assert(sourcemeta::core::idna_property(U'\u200D') ==
///        sourcemeta::core::IDNAProperty::ContextJ);
/// ```
SOURCEMETA_CORE_IDNA_EXPORT
auto idna_property(const char32_t codepoint) noexcept -> IDNAProperty;

/// @ingroup idna
/// Return whether the codepoint at `position` within `label` does not
/// violate any RFC 5892 Appendix A.3-A.9 contextual rule. Returns true
/// vacuously when the codepoint has no such rule. Returns false when
/// `position` is out of range, treated as a precondition violation.
/// See https://www.rfc-editor.org/rfc/rfc5892#appendix-A for the rules.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/idna.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::idna_passes_contexto(U"l\u00B7l", 1));
/// assert(!sourcemeta::core::idna_passes_contexto(U"a\u00B7b", 1));
/// ```
SOURCEMETA_CORE_IDNA_EXPORT
auto idna_passes_contexto(const std::u32string_view label,
                          const std::size_t position) noexcept -> bool;

/// @ingroup idna
/// Return whether the codepoint at `position` within `label` does not
/// violate any RFC 5892 Appendix A.1 / A.2 contextual rule. Returns true
/// vacuously when the codepoint has no such rule. Returns false when
/// `position` is out of range, treated as a precondition violation.
/// See https://www.rfc-editor.org/rfc/rfc5892#appendix-A for the rules.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/idna.h>
/// #include <cassert>
///
/// // Devanagari KA + VIRAMA + ZWJ
/// assert(sourcemeta::core::idna_passes_contextj(U"\u0915\u094D\u200D", 2));
/// // Arabic BEH + ZWNJ + Arabic ALEF
/// assert(sourcemeta::core::idna_passes_contextj(U"\u0628\u200C\u0627", 1));
/// // ZWJ not preceded by Virama
/// assert(!sourcemeta::core::idna_passes_contextj(U"a\u200D", 1));
/// ```
SOURCEMETA_CORE_IDNA_EXPORT
auto idna_passes_contextj(const std::u32string_view label,
                          const std::size_t position) noexcept -> bool;

/// @ingroup idna
/// Return whether the given label satisfies the RFC 5893 Bidi rule. The
/// rule applies to every label of a Bidi domain name (a domain name that
/// contains at least one right-to-left codepoint). The caller is
/// responsible for invoking this only when the domain is a Bidi domain.
/// See https://www.rfc-editor.org/rfc/rfc5893#section-2 for the rule.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/idna.h>
/// #include <cassert>
///
/// // Hebrew letter Alef
/// assert(sourcemeta::core::idna_passes_bidi_rule(U"\u05D0"));
/// // Label starting with a digit (Bidi rule condition 1 violated)
/// assert(!sourcemeta::core::idna_passes_bidi_rule(U"0abc"));
/// ```
SOURCEMETA_CORE_IDNA_EXPORT
auto idna_passes_bidi_rule(const std::u32string_view label) noexcept -> bool;

/// @ingroup idna
/// Return whether the given label is a valid U-label per RFC 5891 §4. See
/// https://www.rfc-editor.org/rfc/rfc5891#section-4 for the criteria.
/// The Bidi rule is not checked here because Bidi domain detection is a
/// property of the whole domain, not of a single label. A pure-ASCII label
/// that satisfies the structural rules is accepted even though it carries no
/// non-ASCII codepoint, so this check is not on its own a guarantee that the
/// label requires IDNA processing. For example:
///
/// ```cpp
/// #include <sourcemeta/core/idna.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::idna_is_valid_u_label(U"d\u00E9j\u00E0"));
/// assert(!sourcemeta::core::idna_is_valid_u_label(U"ab--cd"));
/// assert(!sourcemeta::core::idna_is_valid_u_label(U"-abc"));
/// ```
SOURCEMETA_CORE_IDNA_EXPORT
auto idna_is_valid_u_label(const std::u32string_view label) -> bool;

/// @ingroup idna
/// Return whether the given label is a valid A-label per RFC 5891 §4. See
/// https://www.rfc-editor.org/rfc/rfc5891#section-4 for the criteria.
/// A valid A-label starts with the lowercase ACE prefix "xn--", is pure
/// ASCII, is at most 63 octets, has a non-empty Punycode body that decodes to
/// a U-label containing at least one non-ASCII codepoint, and round-trips
/// through Punycode in its canonical form. Both the prefix and the Punycode
/// body are matched case-sensitively, so an uppercase prefix or a mixed-case
/// body is rejected. This is intended for registration-side validation rather
/// than case-folding lookup. For example:
///
/// ```cpp
/// #include <sourcemeta/core/idna.h>
/// #include <cassert>
///
/// // xn--mnchen-3ya decodes to "München"
/// assert(sourcemeta::core::idna_is_valid_a_label("xn--mnchen-3ya"));
/// // Missing "xn--" prefix
/// assert(!sourcemeta::core::idna_is_valid_a_label("abc"));
/// // Decodes to "abc" (no non-ASCII codepoint)
/// assert(!sourcemeta::core::idna_is_valid_a_label("xn--abc-"));
/// ```
SOURCEMETA_CORE_IDNA_EXPORT
auto idna_is_valid_a_label(const std::string_view label) -> bool;

} // namespace sourcemeta::core

#endif
