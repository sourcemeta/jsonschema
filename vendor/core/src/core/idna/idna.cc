#include <sourcemeta/core/idna.h>

#include <sourcemeta/core/punycode.h>
#include <sourcemeta/core/unicode.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string, std::u32string
#include <string_view> // std::string_view, std::u32string_view

#include "idna_data.h"

namespace sourcemeta::core {

namespace {

// RFC 5890 §2.3.2.1: the maximum length of a label in A-label form
constexpr std::size_t MAXIMUM_LABEL_OCTETS{63};

// Decode and fully validate a Punycode A-label body (the substring after the
// "xn--" prefix), writing the decoded U-label out on success. Returns false
// when the body is not a canonical A-label.
auto validate_a_label_body(const std::string_view encoded,
                           std::u32string &decoded) -> bool {
  if (encoded.empty()) {
    return false;
  }

  try {
    decoded = punycode_to_utf32(encoded);
  } catch (const PunycodeError &) {
    return false;
  }

  // RFC 5890 §2.3.2.1: a U-label contains at least one non-ASCII codepoint.
  // A Punycode body that decodes to pure ASCII is not a real A-label.
  bool has_non_ascii{false};
  for (const auto codepoint : decoded) {
    if (codepoint > 0x7F) {
      has_non_ascii = true;
      break;
    }
  }
  if (!has_non_ascii) {
    return false;
  }

  if (!idna_is_valid_u_label(decoded)) {
    return false;
  }

  // RFC 5891 §4.2: A-labels must be in canonical Punycode form, so
  // re-encoding the decoded U-label must yield the original bytes.
  try {
    return utf32_to_punycode(decoded) == encoded;
  } catch (const PunycodeError &) {
    return false;
  }
}

} // namespace

auto idna_classify_label(const std::u32string_view label,
                         std::u32string &decoded)
    -> std::optional<IDNALabelKind> {
  if (label.empty()) {
    return std::nullopt;
  }

  bool is_pure_ascii{true};
  for (const auto codepoint : label) {
    if (codepoint > 0x7F) {
      is_pure_ascii = false;
      break;
    }
  }

  if (is_pure_ascii) {
    // RFC 5890 §2.3.2.1: the ACE prefix "xn--" is case-insensitive
    const auto has_a_label_prefix{
        label.size() >= 4 && ((label[0] | 0x20) == U'x') &&
        ((label[1] | 0x20) == U'n') && label[2] == U'-' && label[3] == U'-'};

    if (has_a_label_prefix) {
      // RFC 5890 §2.3.2.1: a label in A-label form is at most 63 octets
      if (label.size() > MAXIMUM_LABEL_OCTETS) {
        return std::nullopt;
      }

      std::string ascii;
      ascii.reserve(label.size());
      for (const auto codepoint : label) {
        ascii.push_back(static_cast<char>(codepoint));
      }
      // Normalise the prefix to canonical lowercase before validating, so
      // the round-trip equality does not reject input that only differs in
      // the case of the prefix
      ascii[0] = 'x';
      ascii[1] = 'n';
      if (!validate_a_label_body(
              std::string_view{ascii.data() + 4, ascii.size() - 4}, decoded)) {
        return std::nullopt;
      }
      return IDNALabelKind::ALabel;
    }

    decoded.assign(label.begin(), label.end());
    return IDNALabelKind::Ascii;
  }

  if (!idna_is_valid_u_label(label)) {
    return std::nullopt;
  }
  decoded.assign(label.begin(), label.end());
  return IDNALabelKind::ULabel;
}

auto idna_property(const char32_t codepoint) noexcept -> IDNAProperty {
  if (codepoint > 0x10FFFF) {
    return IDNAProperty::Unassigned;
  }
  const std::size_t page{IDNA_PROPERTY_STAGE1[codepoint >> 10U]};
  return static_cast<IDNAProperty>(
      IDNA_PROPERTY_STAGE2[(page << 10U) | (codepoint & 0x3FFU)]);
}

auto idna_passes_contexto(const std::u32string_view label,
                          const std::size_t position) noexcept -> bool {
  if (position >= label.size()) {
    return false;
  }

  const auto codepoint{label[position]};

  switch (codepoint) {
    // RFC 5892 Appendix A.3 MIDDLE DOT (U+00B7)
    case 0x00B7: {
      if (position == 0 || position + 1 >= label.size()) {
        return false;
      }
      return label[position - 1] == 0x006C && label[position + 1] == 0x006C;
    }

    // RFC 5892 Appendix A.4 GREEK LOWER NUMERAL SIGN (KERAIA) (U+0375)
    case 0x0375: {
      if (position + 1 >= label.size()) {
        return false;
      }
      return script(label[position + 1]) == UnicodeScript::Greek;
    }

    // RFC 5892 Appendix A.5 HEBREW PUNCTUATION GERESH (U+05F3)
    // RFC 5892 Appendix A.6 HEBREW PUNCTUATION GERSHAYIM (U+05F4)
    case 0x05F3:
    case 0x05F4: {
      if (position == 0) {
        return false;
      }
      return script(label[position - 1]) == UnicodeScript::Hebrew;
    }

    // RFC 5892 Appendix A.7 KATAKANA MIDDLE DOT (U+30FB)
    case 0x30FB: {
      for (const auto other : label) {
        const auto other_script{script(other)};
        if (other_script == UnicodeScript::Hiragana ||
            other_script == UnicodeScript::Katakana ||
            other_script == UnicodeScript::Han) {
          return true;
        }
      }
      return false;
    }

    default:
      break;
  }

  // RFC 5892 Appendix A.8 ARABIC-INDIC DIGITS (U+0660..U+0669) and
  // Appendix A.9 EXTENDED ARABIC-INDIC DIGITS (U+06F0..U+06F9): a label must
  // not mix the two blocks. A single scan over the label settles both rules.
  const bool is_arabic_indic{codepoint >= 0x0660 && codepoint <= 0x0669};
  const bool is_extended_arabic_indic{codepoint >= 0x06F0 &&
                                      codepoint <= 0x06F9};
  if (is_arabic_indic || is_extended_arabic_indic) {
    bool has_arabic_indic{false};
    bool has_extended_arabic_indic{false};
    for (const auto other : label) {
      if (other >= 0x0660 && other <= 0x0669) {
        has_arabic_indic = true;
      } else if (other >= 0x06F0 && other <= 0x06F9) {
        has_extended_arabic_indic = true;
      }
    }
    return !(has_arabic_indic && has_extended_arabic_indic);
  }

  // No RFC 5892 Appendix A.3-A.9 rule applies to this codepoint, so there
  // is nothing to violate.
  return true;
}

auto idna_passes_contextj(const std::u32string_view label,
                          const std::size_t position) noexcept -> bool {
  if (position >= label.size()) {
    return false;
  }

  const auto codepoint{label[position]};

  // RFC 5892 Appendix A.2 ZERO WIDTH JOINER (U+200D)
  if (codepoint == 0x200D) {
    if (position == 0) {
      return false;
    }
    return combining_class(label[position - 1]) == 9;
  }

  // RFC 5892 Appendix A.1 ZERO WIDTH NON-JOINER (U+200C)
  if (codepoint == 0x200C) {
    if (position > 0 && combining_class(label[position - 1]) == 9) {
      return true;
    }

    bool has_left{false};
    for (std::size_t index = position; index-- > 0;) {
      const auto joining{joining_type(label[index])};
      if (joining == JoiningType::Transparent) {
        continue;
      }
      has_left = joining == JoiningType::LeftJoining ||
                 joining == JoiningType::DualJoining;
      break;
    }
    if (!has_left) {
      return false;
    }

    for (std::size_t index = position + 1; index < label.size(); ++index) {
      const auto joining{joining_type(label[index])};
      if (joining == JoiningType::Transparent) {
        continue;
      }
      return joining == JoiningType::RightJoining ||
             joining == JoiningType::DualJoining;
    }
    return false;
  }

  // No RFC 5892 Appendix A.1 / A.2 rule applies to this codepoint, so there
  // is nothing to violate.
  return true;
}

auto idna_is_valid_u_label(const std::u32string_view label) -> bool {
  if (label.empty()) {
    return false;
  }

  // RFC 5891 §4.1.2.A: a U-label must be in Unicode Normalisation Form C
  if (!is_nfc(label)) {
    return false;
  }

  // RFC 5891 §4.2.3.1: must not start or end with a hyphen, and must not
  // have a hyphen in both positions 3 and 4 (the IDNA A-label prefix
  // shape "xn--" must not appear in U-labels).
  if (label.front() == U'-' || label.back() == U'-') {
    return false;
  }
  if (label.size() >= 4 && label[2] == U'-' && label[3] == U'-') {
    return false;
  }

  // RFC 5891 §4.2.3.2: must not start with a combining mark.
  if (is_combining_mark(label.front())) {
    return false;
  }

  // RFC 5891 §4.2.3.3: every codepoint must be PVALID or satisfy its
  // CONTEXTJ / CONTEXTO contextual rule. DISALLOWED and UNASSIGNED reject.
  for (std::size_t position = 0; position < label.size(); ++position) {
    switch (idna_property(label[position])) {
      case IDNAProperty::PValid:
        break;
      case IDNAProperty::ContextJ:
        if (!idna_passes_contextj(label, position)) {
          return false;
        }
        break;
      case IDNAProperty::ContextO:
        if (!idna_passes_contexto(label, position)) {
          return false;
        }
        break;
      case IDNAProperty::Disallowed:
      case IDNAProperty::Unassigned:
        return false;
    }
  }

  // RFC 5890 §2.3.2.1: the corresponding A-label (the "xn--" prefix plus the
  // Punycode-encoded body) must not exceed 63 octets
  try {
    if (4 + utf32_to_punycode(label).size() > MAXIMUM_LABEL_OCTETS) {
      return false;
    }
  } catch (const PunycodeError &) {
    return false;
  }

  return true;
}

auto idna_passes_bidi_rule(const std::u32string_view label) noexcept -> bool {
  if (label.empty()) {
    return false;
  }

  // RFC 5893 §2 condition 1: the first character must be L, R, or AL.
  const auto first_class{bidi_class(label[0])};
  bool is_rtl_label{false};
  if (first_class == BidiClass::LeftToRight) {
    is_rtl_label = false;
  } else if (first_class == BidiClass::RightToLeft ||
             first_class == BidiClass::ArabicLetter) {
    is_rtl_label = true;
  } else {
    return false;
  }

  bool has_european_number{false};
  bool has_arabic_number{false};

  // RFC 5893 §2 conditions 2 and 5: only specific classes are allowed in
  // each label direction.
  for (const auto codepoint : label) {
    const auto class_value{bidi_class(codepoint)};
    if (is_rtl_label) {
      switch (class_value) {
        case BidiClass::RightToLeft:
        case BidiClass::ArabicLetter:
        case BidiClass::ArabicNumber:
        case BidiClass::EuropeanNumber:
        case BidiClass::EuropeanSeparator:
        case BidiClass::CommonSeparator:
        case BidiClass::EuropeanTerminator:
        case BidiClass::OtherNeutral:
        case BidiClass::BoundaryNeutral:
        case BidiClass::NonspacingMark:
          break;
        default:
          return false;
      }
      if (class_value == BidiClass::EuropeanNumber) {
        has_european_number = true;
      } else if (class_value == BidiClass::ArabicNumber) {
        has_arabic_number = true;
      }
    } else {
      switch (class_value) {
        case BidiClass::LeftToRight:
        case BidiClass::EuropeanNumber:
        case BidiClass::EuropeanSeparator:
        case BidiClass::CommonSeparator:
        case BidiClass::EuropeanTerminator:
        case BidiClass::OtherNeutral:
        case BidiClass::BoundaryNeutral:
        case BidiClass::NonspacingMark:
          break;
        default:
          return false;
      }
    }
  }

  // RFC 5893 §2 condition 4 (RTL labels only): cannot have both EN and AN.
  // The "and vice versa" wording in the RFC is internal to the RTL branch
  // (EN/AN exclusivity within RTL), not symmetry to LTR.
  if (is_rtl_label && has_european_number && has_arabic_number) {
    return false;
  }

  // RFC 5893 §2 conditions 3 and 6: the label must end with a specific
  // class, optionally followed by NSM characters.
  for (std::size_t index = label.size(); index-- > 0;) {
    const auto class_value{bidi_class(label[index])};
    if (class_value == BidiClass::NonspacingMark) {
      continue;
    }
    if (is_rtl_label) {
      return class_value == BidiClass::RightToLeft ||
             class_value == BidiClass::ArabicLetter ||
             class_value == BidiClass::EuropeanNumber ||
             class_value == BidiClass::ArabicNumber;
    }
    return class_value == BidiClass::LeftToRight ||
           class_value == BidiClass::EuropeanNumber;
  }

  return false;
}

auto idna_is_valid_a_label(const std::string_view label) -> bool {
  constexpr std::string_view prefix{"xn--"};
  if (!label.starts_with(prefix)) {
    return false;
  }

  // RFC 5890 §2.3.2.1: a label in A-label form is at most 63 octets
  if (label.size() > MAXIMUM_LABEL_OCTETS) {
    return false;
  }

  // RFC 5890 §2.3.2.1: A-labels are pure ASCII
  for (const auto byte : label) {
    if (static_cast<unsigned char>(byte) > 0x7F) {
      return false;
    }
  }

  // The substring after the prefix. Constructing the view via (data, size)
  // avoids `std::string_view::substr`, which is not noexcept.
  const std::string_view encoded{label.data() + prefix.size(),
                                 label.size() - prefix.size()};

  std::u32string decoded;
  return validate_a_label_body(encoded, decoded);
}

} // namespace sourcemeta::core
