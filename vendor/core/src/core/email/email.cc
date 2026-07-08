#include <sourcemeta/core/email.h>

#include <sourcemeta/core/dns.h>
#include <sourcemeta/core/unicode.h>

#include "helpers.h"

namespace sourcemeta::core {

// RFC 5321 §4.1.2 Mailbox grammar. When AllowUtf8 is true, RFC 6531 §3.3
// extends atext, qtextSMTP, and sub-domain with UTF8-non-ascii alternatives.
// When UseUts46 is also true, the domain is validated under UTS #46 processing
// rather than strict IDNA 2008.
template <bool AllowUtf8, bool UseUts46 = false>
static auto is_mailbox(const std::string_view value) -> bool {
  if (value.empty()) {
    return false;
  }

  // RFC 5321 §4.5.3.1.3: a path is at most 256 octets including the enclosing
  // angle brackets, so the mailbox it carries is at most 254
  if (value.size() > 254) {
    return false;
  }

  std::string_view::size_type position{0};

  if (value[0] == '"') {
    // RFC 5321 §4.1.2: Quoted-string = DQUOTE *QcontentSMTP DQUOTE
    position = 1;
    while (position < value.size() && value[position] != '"') {
      if (value[position] == '\\') {
        // RFC 5321 §4.1.2: quoted-pairSMTP = %d92 %d32-126
        position += 1;
        if (position >= value.size()) {
          return false;
        }
        const auto body{static_cast<unsigned char>(value[position])};
        if (body < 32 || body > 126) {
          return false;
        }
        position += 1;
        continue;
      }

      if (is_qtext_smtp(static_cast<unsigned char>(value[position]))) {
        position += 1;
        continue;
      }

      if constexpr (AllowUtf8) {
        // RFC 6531 §3.3: qtextSMTP =/ UTF8-non-ascii
        const auto utf8_length{utf8_codepoint_length(value, position)};
        if (utf8_length < 2) {
          return false;
        }
        position += utf8_length;
      } else {
        return false;
      }
    }
    if (position >= value.size()) {
      return false;
    }
    // value[position] is the closing DQUOTE
    position += 1;
  } else {
    // RFC 5321 §4.1.2: Dot-string = Atom *("." Atom), Atom = 1*atext
    bool previous_was_dot{false};
    bool atom_started{false};
    while (position < value.size() && value[position] != '@') {
      const auto character{value[position]};
      if (character == '.') {
        if (!atom_started || previous_was_dot) {
          return false;
        }
        previous_was_dot = true;
        atom_started = false;
        position += 1;
        continue;
      }

      if (is_atext(character)) {
        previous_was_dot = false;
        atom_started = true;
        position += 1;
        continue;
      }

      if constexpr (AllowUtf8) {
        // RFC 6531 §3.3: atext =/ UTF8-non-ascii
        const auto utf8_length{utf8_codepoint_length(value, position)};
        if (utf8_length < 2) {
          return false;
        }
        previous_was_dot = false;
        atom_started = true;
        position += utf8_length;
      } else {
        return false;
      }
    }
    if (position == 0 || previous_was_dot) {
      return false;
    }
  }

  // RFC 5321 §4.5.3.1.1: Local-part octet limit is 64
  if (position > 64) {
    return false;
  }

  // RFC 5321 §4.1.2: Mailbox = Local-part "@" ( Domain / address-literal )
  if (position >= value.size() || value[position] != '@') {
    return false;
  }

  const auto domain{value.substr(position + 1)};

  // RFC 5321 §4.1.3: address-literal = "[" ( IPv4 / IPv6 / General ) "]"
  if (!domain.empty() && domain.front() == '[') {
    return is_address_literal(domain);
  }

  if constexpr (AllowUtf8) {
    // RFC 6531 §3.3: sub-domain =/ U-label
    if constexpr (UseUts46) {
      return is_idn_hostname_uts46(domain);
    } else {
      return is_idn_hostname(domain);
    }
  } else {
    // RFC 5321 §4.1.2 Domain matches is_hostname (RFC 1123 §2.1) by
    // grammar, by 63-octet label cap (RFC 1035 §2.3.4), and by
    // 255-octet total cap (RFC 5321 §4.5.3.1.2)
    return is_hostname(domain);
  }
}

auto is_email(const std::string_view value) -> bool {
  return is_mailbox<false>(value);
}

auto is_idn_email(const std::string_view value) -> bool {
  return is_mailbox<true>(value);
}

auto is_idn_email_uts46(const std::string_view value) -> bool {
  return is_mailbox<true, true>(value);
}

} // namespace sourcemeta::core
