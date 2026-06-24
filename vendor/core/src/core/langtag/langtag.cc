#include <sourcemeta/core/langtag.h>
#include <sourcemeta/core/text.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t
#include <string_view> // std::string_view

namespace {

// The irregular grandfathered tags (RFC 5646 Section 2.2.8) do not fit the
// langtag grammar and must be matched literally. The regular grandfathered
// tags are intentionally omitted, as they are already accepted by the langtag
// grammar.
constexpr std::array<std::string_view, 17> irregular_grandfathered{
    {"en-GB-oed", "i-ami", "i-bnn", "i-default", "i-enochian", "i-hak",
     "i-klingon", "i-lux", "i-mingo", "i-navajo", "i-pwn", "i-tao", "i-tay",
     "i-tsu", "sgn-BE-FR", "sgn-BE-NL", "sgn-CH-DE"}};

// The subtag starting at the given position, up to the next hyphen or the end.
auto subtag_at(const std::string_view value, const std::size_t position)
    -> std::string_view {
  auto end{position};
  while (end < value.size() && value[end] != '-') {
    end += 1;
  }
  return value.substr(position, end - position);
}

// Advance past a subtag of the given length, and the separating hyphen if any.
auto advance(const std::string_view value, std::size_t &position,
             const std::size_t length) -> void {
  position += length;
  if (position < value.size()) {
    position += 1;
  }
}

// singleton = DIGIT / a-w / y-z, mapped to the range [0, 35]
auto singleton_index(const char character) -> std::size_t {
  const auto lowered{sourcemeta::core::to_lowercase(character)};
  if (sourcemeta::core::is_digit(lowered)) {
    return static_cast<std::size_t>(lowered - '0');
  }
  return static_cast<std::size_t>(lowered - 'a') + 10;
}

// language = 2*3ALPHA ["-" extlang] / 4ALPHA / 5*8ALPHA
auto parse_language(const std::string_view value, std::size_t &position)
    -> bool {
  const auto primary{subtag_at(value, position)};
  if (!sourcemeta::core::is_alpha(primary)) {
    return false;
  }
  if (primary.size() >= 2 && primary.size() <= 3) {
    advance(value, position, primary.size());
    // extlang = 3ALPHA *2("-" 3ALPHA), so up to three subtags of three letters
    for (std::size_t count{0}; count < 3; count += 1) {
      const auto extlang{subtag_at(value, position)};
      if (extlang.size() != 3 || !sourcemeta::core::is_alpha(extlang)) {
        break;
      }
      advance(value, position, 3);
    }
    return true;
  }
  if (primary.size() == 4 || (primary.size() >= 5 && primary.size() <= 8)) {
    advance(value, position, primary.size());
    return true;
  }
  return false;
}

// script = 4ALPHA
auto parse_script(const std::string_view value, std::size_t &position) -> void {
  const auto script{subtag_at(value, position)};
  if (script.size() == 4 && sourcemeta::core::is_alpha(script)) {
    advance(value, position, 4);
  }
}

// region = 2ALPHA / 3DIGIT
auto parse_region(const std::string_view value, std::size_t &position) -> void {
  const auto region{subtag_at(value, position)};
  if ((region.size() == 2 && sourcemeta::core::is_alpha(region)) ||
      (region.size() == 3 && sourcemeta::core::is_digit(region))) {
    advance(value, position, region.size());
  }
}

// Whether the candidate variant already appears among the subtags in
// [begin, end). The variant region is contiguous, so it is re-scanned rather
// than stored.
auto seen_variant(const std::string_view value, const std::size_t begin,
                  const std::size_t end, const std::string_view candidate)
    -> bool {
  std::size_t cursor{begin};
  while (cursor < end) {
    auto stop{cursor};
    while (stop < end && value[stop] != '-') {
      stop += 1;
    }
    if (sourcemeta::core::equals_ignore_case(
            value.substr(cursor, stop - cursor), candidate)) {
      return true;
    }
    cursor = stop + 1;
  }
  return false;
}

// variant = 5*8alphanum / (DIGIT 3alphanum), with no repeats
auto parse_variants(const std::string_view value, std::size_t &position)
    -> bool {
  const auto begin{position};
  while (true) {
    const auto variant{subtag_at(value, position)};
    const bool matches{(variant.size() >= 5 && variant.size() <= 8 &&
                        sourcemeta::core::is_alphanum(variant)) ||
                       (variant.size() == 4 &&
                        sourcemeta::core::is_digit(variant.front()) &&
                        sourcemeta::core::is_alphanum(variant))};
    if (!matches) {
      break;
    }
    if (seen_variant(value, begin, position, variant)) {
      return false;
    }
    advance(value, position, variant.size());
  }
  return true;
}

// Consume one or more alphanumeric subtags whose length is in
// [minimum_length, 8], requiring at least one.
auto consume_subtags(const std::string_view value, std::size_t &position,
                     const std::size_t minimum_length) -> bool {
  std::size_t count{0};
  while (true) {
    const auto subtag{subtag_at(value, position)};
    if (subtag.size() < minimum_length || subtag.size() > 8 ||
        !sourcemeta::core::is_alphanum(subtag)) {
      break;
    }
    advance(value, position, subtag.size());
    count += 1;
  }
  return count > 0;
}

// extension = singleton 1*("-" (2*8alphanum)), with each singleton at most
// once. The singleton "x" is excluded as it introduces the private use.
auto parse_extensions(const std::string_view value, std::size_t &position)
    -> bool {
  std::uint64_t seen{0};
  while (true) {
    const auto singleton{subtag_at(value, position)};
    if (singleton.size() != 1) {
      break;
    }
    const auto character{singleton.front()};
    if (!sourcemeta::core::is_alphanum(character) ||
        sourcemeta::core::to_lowercase(character) == 'x') {
      break;
    }
    const auto bit{std::uint64_t{1} << singleton_index(character)};
    if ((seen & bit) != 0) {
      return false;
    }
    seen |= bit;
    advance(value, position, 1);
    if (!consume_subtags(value, position, 2)) {
      return false;
    }
  }
  return true;
}

// privateuse = "x" 1*("-" (1*8alphanum))
auto parse_privateuse(const std::string_view value, std::size_t &position)
    -> bool {
  const auto singleton{subtag_at(value, position)};
  if (singleton.size() != 1 ||
      sourcemeta::core::to_lowercase(singleton.front()) != 'x') {
    return true;
  }
  advance(value, position, 1);
  return consume_subtags(value, position, 1);
}

// Language-Tag = langtag (RFC 5646 Section 2.1)
auto valid_langtag(const std::string_view value) -> bool {
  std::size_t position{0};
  if (!parse_language(value, position)) {
    return false;
  }
  parse_script(value, position);
  parse_region(value, position);
  if (!parse_variants(value, position)) {
    return false;
  }
  if (!parse_extensions(value, position)) {
    return false;
  }
  if (!parse_privateuse(value, position)) {
    return false;
  }
  return position >= value.size();
}

// Language-Tag = privateuse (RFC 5646 Section 2.1)
auto valid_privateuse(const std::string_view value) -> bool {
  std::size_t position{0};
  const auto singleton{subtag_at(value, position)};
  if (singleton.size() != 1 ||
      sourcemeta::core::to_lowercase(singleton.front()) != 'x') {
    return false;
  }
  advance(value, position, 1);
  return consume_subtags(value, position, 1) && position >= value.size();
}

auto is_irregular_grandfathered(const std::string_view value) -> bool {
  for (const auto entry : irregular_grandfathered) {
    if (sourcemeta::core::equals_ignore_case(value, entry)) {
      return true;
    }
  }
  return false;
}

} // namespace

namespace sourcemeta::core {

auto is_langtag(const std::string_view value) -> bool {
  if (value.empty() || value.front() == '-' || value.back() == '-') {
    return false;
  }
  // Language-Tag = langtag / privateuse / grandfathered. The regular
  // grandfathered tags are covered by the langtag alternative, so only the
  // irregular ones need a literal fallback.
  if (valid_langtag(value)) {
    return true;
  }
  if (valid_privateuse(value)) {
    return true;
  }
  return is_irregular_grandfathered(value);
}

} // namespace sourcemeta::core
