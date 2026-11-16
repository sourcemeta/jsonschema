#include <sourcemeta/core/uritemplate.h>

#include "helpers.h"

#include <cassert> // assert
#include <utility> // std::pair
#include <vector>  // std::vector

namespace sourcemeta::core {

template <typename T, bool CheckOnly>
static auto try_parse(std::string_view &remaining, std::size_t &offset,
                      std::vector<URITemplateToken> *tokens) -> bool {
  if (auto result = parse_expression<T, CheckOnly>(remaining)) {
    if constexpr (CheckOnly) {
      remaining.remove_prefix(*result);
      offset += *result;
    } else {
      tokens->emplace_back(std::move(result->first));
      remaining.remove_prefix(result->second);
      offset += result->second;
    }
    return true;
  }

  return false;
}

// Each of the alternatives either takes what it reads or refuses it outright,
// and a literal answers for everything that opens no expression, so it is read
// rather than tried once none of them has taken anything
template <bool CheckOnly, typename... Ts>
static auto parse_any(std::string_view &remaining, std::size_t &offset,
                      std::vector<URITemplateToken> *tokens) -> void {
  if ((try_parse<Ts, CheckOnly>(remaining, offset, tokens) || ...)) {
    return;
  }

  if constexpr (CheckOnly) {
    const auto length{parse_literal<true>(remaining)};
    remaining.remove_prefix(length);
    offset += length;
  } else {
    auto literal{parse_literal<false>(remaining)};
    tokens->emplace_back(std::move(literal.first));
    remaining.remove_prefix(literal.second);
    offset += literal.second;
  }
}

URITemplate::URITemplate(const std::string_view source) {
  std::string_view remaining{source};
  std::size_t offset = 0;

  while (!remaining.empty()) {
    try {
      parse_any<
          false, URITemplateTokenReservedExpansion,
          URITemplateTokenFragmentExpansion, URITemplateTokenLabelExpansion,
          URITemplateTokenPathExpansion, URITemplateTokenPathParameterExpansion,
          URITemplateTokenQueryExpansion,
          URITemplateTokenQueryContinuationExpansion, URITemplateTokenVariable>(
          remaining, offset, &this->tokens_);
    } catch (URITemplateParseError &error) {
      throw URITemplateParseError(offset + error.column());
    }
  }
}

auto URITemplate::is_literal(const char32_t codepoint) noexcept -> bool {
  return is_literal_codepoint(codepoint);
}

auto URITemplate::is_uritemplate(const std::string_view input) noexcept
    -> bool {
  try {
    std::string_view remaining{input};
    std::size_t offset = 0;
    while (!remaining.empty()) {
      parse_any<
          true, URITemplateTokenReservedExpansion,
          URITemplateTokenFragmentExpansion, URITemplateTokenLabelExpansion,
          URITemplateTokenPathExpansion, URITemplateTokenPathParameterExpansion,
          URITemplateTokenQueryExpansion,
          URITemplateTokenQueryContinuationExpansion, URITemplateTokenVariable>(
          remaining, offset, nullptr);
    }
    return true;
  } catch (...) {
    return false;
  }
}

auto URITemplate::size() const noexcept -> std::uint64_t {
  return static_cast<std::uint64_t>(this->tokens_.size());
}

auto URITemplate::empty() const noexcept -> bool {
  return this->tokens_.empty();
}

auto URITemplate::at(const std::size_t index) const & -> const
    URITemplateToken & {
  assert(index < this->tokens_.size());
  return this->tokens_[index];
}

auto URITemplate::at(const std::size_t index) && -> URITemplateToken {
  assert(index < this->tokens_.size());
  return std::move(this->tokens_[index]);
}

auto URITemplate::begin() const noexcept
    -> std::vector<URITemplateToken>::const_iterator {
  return this->tokens_.cbegin();
}

auto URITemplate::end() const noexcept
    -> std::vector<URITemplateToken>::const_iterator {
  return this->tokens_.cend();
}

auto URITemplate::expand(
    const std::function<URITemplateValue(std::string_view name)> &callback,
    const URITemplateExpansionMode mode) const -> std::string {
  std::string result;

  for (const auto &token : this->tokens_) {
    std::visit(
        [&result, &callback, mode](const auto &expansion) -> void {
          using T = std::decay_t<decltype(expansion)>;
          if constexpr (std::is_same_v<T, URITemplateTokenLiteral>) {
            result += expansion.value;
          } else {
            expand_expression<T>(result, expansion.variables, callback, mode);
          }
        },
        token);
  }

  return result;
}

} // namespace sourcemeta::core
