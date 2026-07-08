#include <sourcemeta/core/dns.h>

#include <sourcemeta/core/idna.h>
#include <sourcemeta/core/punycode.h>
#include <sourcemeta/core/unicode.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <string>      // std::string, std::u32string
#include <string_view> // std::string_view, std::u32string_view
#include <vector>      // std::vector

namespace sourcemeta::core {

// UTS #46 §3.1: the four IDNA label separators (FULL STOP, IDEOGRAPHIC
// FULL STOP, FULLWIDTH FULL STOP, HALFWIDTH IDEOGRAPHIC FULL STOP)
static constexpr auto is_idna_label_separator(const char32_t codepoint)
    -> bool {
  return codepoint == U'.' || codepoint == U'\u3002' ||
         codepoint == U'\uFF0E' || codepoint == U'\uFF61';
}

namespace {

// Validate an already-decoded presentation-form domain name against the
// IDNA 2008 rules (RFC 5890-5893). The caller is responsible for any prior
// transformation, such as the UTS #46 mapping.
auto validate_idn_labels(const std::u32string_view codepoints) -> bool {
  if (codepoints.empty()) {
    return false;
  }

  // UTS #46 §3.1: a leading or trailing separator means an empty label
  if (is_idna_label_separator(codepoints.front()) ||
      is_idna_label_separator(codepoints.back())) {
    return false;
  }

  std::vector<std::u32string> decoded_labels;
  // RFC 1035 §3.1: presentation form ≤ 253 octets in A-label form
  std::size_t total_octets{0};
  std::size_t label_start{0};
  for (std::size_t position = 0; position <= codepoints.size(); ++position) {
    const bool at_end{position == codepoints.size()};
    if (!at_end && !is_idna_label_separator(codepoints[position])) {
      continue;
    }
    const std::u32string_view label{codepoints.data() + label_start,
                                    position - label_start};

    std::u32string decoded;
    const auto kind{idna_classify_label(label, decoded)};
    if (!kind.has_value()) {
      return false;
    }

    // The label's A-label form octet count: for Ascii and ALabel kinds the
    // input is already in A-label form, so codepoint count equals octet
    // count. For ULabel kinds we Punycode-encode the U-label to measure
    // the corresponding A-label.
    std::size_t a_label_octets{label.size()};
    if (*kind == IDNALabelKind::ULabel) {
      try {
        const auto body{utf32_to_punycode(decoded)};
        a_label_octets = 4 + body.size();
      } catch (const PunycodeError &) {
        return false;
      }
    } else if (*kind == IDNALabelKind::Ascii) {
      // RFC 5891 §4.2.3.1: a label must not contain "--" in the third and
      // fourth positions unless it is a valid A-label (an "xn--" ACE label,
      // which is classified as ALabel rather than Ascii). This matches the
      // U-label path, which already rejects such labels.
      if (label.size() >= 4 && label[2] == U'-' && label[3] == U'-') {
        return false;
      }
      // RFC 1123 §2.1: ASCII labels still need the LDH grammar check
      std::string ascii;
      ascii.reserve(label.size());
      for (const auto codepoint : label) {
        ascii.push_back(static_cast<char>(codepoint));
      }
      if (!is_hostname(ascii)) {
        return false;
      }
    }

    // RFC 5891 §4.2.4 / RFC 1035 §2.3.4: each label, in A-label form,
    // must be 1-63 octets
    if (a_label_octets > 63) {
      return false;
    }

    if (!decoded_labels.empty()) {
      total_octets += 1;
    }
    total_octets += a_label_octets;
    if (total_octets > 253) {
      return false;
    }

    decoded_labels.push_back(std::move(decoded));
    label_start = position + 1;
  }

  // RFC 5893 §1.4: a Bidi domain name has at least one codepoint with
  // Bidi_Class R, AL, or AN
  bool is_bidi_domain{false};
  for (const auto &label : decoded_labels) {
    for (const auto codepoint : label) {
      const auto class_value{bidi_class(codepoint)};
      if (class_value == BidiClass::RightToLeft ||
          class_value == BidiClass::ArabicLetter ||
          class_value == BidiClass::ArabicNumber) {
        is_bidi_domain = true;
        break;
      }
    }
    if (is_bidi_domain) {
      break;
    }
  }

  // RFC 5893 §2: every label of a Bidi domain must satisfy the Bidi rule
  if (is_bidi_domain) {
    for (const auto &label : decoded_labels) {
      if (!idna_passes_bidi_rule(label)) {
        return false;
      }
    }
  }

  return true;
}

} // namespace

auto is_idn_hostname(const std::string_view value) -> bool {
  // This is strict IDNA 2008 validation: the input is validated as-is, with
  // no mapping pass. Callers that need the UTS #46 lookup behaviour (case
  // folding, deviation handling, ignorable removal) should use
  // `is_idn_hostname_uts46`, which is what the `vendor/unicodetools`
  // IdnaTestV2.txt corpus exercises.
  const auto codepoints{utf8_to_utf32(value)};
  if (!codepoints.has_value()) {
    return false;
  }

  return validate_idn_labels(*codepoints);
}

auto is_idn_hostname_uts46(const std::string_view value) -> bool {
  const auto codepoints{utf8_to_utf32(value)};
  if (!codepoints.has_value()) {
    return false;
  }

  // UTS #46 §4 steps 1-2: map and normalise before validating. Disallowed
  // codepoints are preserved by the mapping and rejected here by the per-label
  // validity check.
  return validate_idn_labels(idna_uts46_map(*codepoints));
}

} // namespace sourcemeta::core
