#include <sourcemeta/core/unicode.h>

#include <algorithm>   // std::stable_sort
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string>      // std::u32string
#include <string_view> // std::u32string_view

namespace sourcemeta::core {

namespace {

// UAX #15 §3.12 Hangul algorithm constants
constexpr char32_t HANGUL_SBASE{0xAC00};
constexpr char32_t HANGUL_LBASE{0x1100};
constexpr char32_t HANGUL_VBASE{0x1161};
constexpr char32_t HANGUL_TBASE{0x11A7};
constexpr std::size_t HANGUL_LCOUNT{19};
constexpr std::size_t HANGUL_VCOUNT{21};
constexpr std::size_t HANGUL_TCOUNT{28};
constexpr std::size_t HANGUL_NCOUNT{HANGUL_VCOUNT * HANGUL_TCOUNT};
constexpr std::size_t HANGUL_SCOUNT{HANGUL_LCOUNT * HANGUL_NCOUNT};

auto canonical_decompose_recursive(const char32_t codepoint,
                                   std::u32string &output) -> void {
  // UAX #15 §3.12: Hangul precomposed syllables decompose algorithmically
  // into an L jamo, a V jamo, and optionally a T jamo
  if (codepoint >= HANGUL_SBASE && codepoint < HANGUL_SBASE + HANGUL_SCOUNT) {
    const auto index{static_cast<std::size_t>(codepoint - HANGUL_SBASE)};
    output.push_back(
        static_cast<char32_t>(HANGUL_LBASE + index / HANGUL_NCOUNT));
    output.push_back(static_cast<char32_t>(
        HANGUL_VBASE + (index % HANGUL_NCOUNT) / HANGUL_TCOUNT));
    const auto t_index{index % HANGUL_TCOUNT};
    if (t_index != 0) {
      output.push_back(static_cast<char32_t>(HANGUL_TBASE + t_index));
    }
    return;
  }

  const auto decomposition{canonical_decomposition(codepoint)};
  if (decomposition.empty()) {
    output.push_back(codepoint);
    return;
  }

  for (const auto component : decomposition) {
    canonical_decompose_recursive(component, output);
  }
}

auto canonical_reorder(std::u32string &buffer) -> void {
  std::size_t index{0};
  while (index < buffer.size()) {
    if (combining_class(buffer[index]) == 0) {
      index += 1;
      continue;
    }
    std::size_t end{index + 1};
    while (end < buffer.size() && combining_class(buffer[end]) != 0) {
      end += 1;
    }
    std::stable_sort(buffer.begin() + static_cast<std::ptrdiff_t>(index),
                     buffer.begin() + static_cast<std::ptrdiff_t>(end),
                     [](const char32_t left, const char32_t right) noexcept {
                       return combining_class(left) < combining_class(right);
                     });
    index = end;
  }
}

// UAX #15 §3.12: Hangul L + V composes to LV, and an LV syllable plus a
// T jamo composes to LVT. Other pairs fall through to the static table.
auto try_compose(const char32_t starter, const char32_t combining) noexcept
    -> std::optional<char32_t> {
  if (starter >= HANGUL_LBASE && starter < HANGUL_LBASE + HANGUL_LCOUNT &&
      combining >= HANGUL_VBASE && combining < HANGUL_VBASE + HANGUL_VCOUNT) {
    const auto l_index{static_cast<std::size_t>(starter - HANGUL_LBASE)};
    const auto v_index{static_cast<std::size_t>(combining - HANGUL_VBASE)};
    return static_cast<char32_t>(
        HANGUL_SBASE + (l_index * HANGUL_VCOUNT + v_index) * HANGUL_TCOUNT);
  }

  if (starter >= HANGUL_SBASE && starter < HANGUL_SBASE + HANGUL_SCOUNT &&
      (starter - HANGUL_SBASE) % HANGUL_TCOUNT == 0 &&
      combining > HANGUL_TBASE && combining < HANGUL_TBASE + HANGUL_TCOUNT) {
    return static_cast<char32_t>(starter + (combining - HANGUL_TBASE));
  }

  return canonical_composition(starter, combining);
}

// UAX #15 §3 Canonical Composition Algorithm: walk left to right and
// greedily combine each starter with following non-blocked codepoints.
// Expects the input to have already been canonically decomposed and
// reordered.
auto canonical_compose_pass(const std::u32string_view input) -> std::u32string {
  std::u32string result;
  result.reserve(input.size());

  std::size_t last_starter{std::u32string::npos};
  std::uint8_t last_ccc{0};

  for (const auto codepoint : input) {
    const auto ccc{combining_class(codepoint)};

    if (last_starter != std::u32string::npos) {
      // A starter can only compose with the immediately preceding starter
      // (no intervening combining marks). A combining mark can compose
      // with the last starter only when every intervening mark has lower
      // CCC than itself, which after canonical reordering reduces to the
      // most recently appended mark
      const bool unblocked{ccc == 0 ? last_ccc == 0 : last_ccc < ccc};
      if (unblocked) {
        const auto composed{try_compose(result[last_starter], codepoint)};
        if (composed.has_value()) {
          result[last_starter] = *composed;
          continue;
        }
      }
    }

    if (ccc == 0) {
      last_starter = result.size();
      last_ccc = 0;
    } else {
      last_ccc = ccc;
    }
    result.push_back(codepoint);
  }

  return result;
}

} // namespace

auto nfc(const std::u32string_view input) -> std::u32string {
  std::u32string decomposed;
  decomposed.reserve(input.size());
  for (const auto codepoint : input) {
    canonical_decompose_recursive(codepoint, decomposed);
  }
  canonical_reorder(decomposed);
  return canonical_compose_pass(decomposed);
}

auto is_nfc(const std::u32string_view input) -> bool {
  // UAX #15 §9.1 quick check: a single linear pass that returns Yes or No
  // for the overwhelming majority of inputs. Only `Maybe` falls through
  // to the full normalise-and-compare slow path.
  auto result{NFCQuickCheck::Yes};
  std::uint8_t last_ccc{0};
  for (const auto codepoint : input) {
    const auto ccc{combining_class(codepoint)};
    if (ccc != 0 && last_ccc > ccc) {
      return false;
    }
    const auto check{nfc_quick_check(codepoint)};
    if (check == NFCQuickCheck::No) {
      return false;
    }
    if (check == NFCQuickCheck::Maybe) {
      result = NFCQuickCheck::Maybe;
    }
    last_ccc = ccc;
  }
  if (result == NFCQuickCheck::Yes) {
    return true;
  }
  return nfc(input) == input;
}

} // namespace sourcemeta::core
