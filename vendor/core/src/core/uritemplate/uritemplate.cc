#include <sourcemeta/core/uritemplate.h>

#include "helpers.h"

#include <cassert> // assert
#include <utility> // std::pair
#include <vector>  // std::vector

namespace sourcemeta::core {

template <typename T>
static auto try_parse(std::string_view &remaining, std::size_t &offset,
                      std::vector<URITemplateToken> &tokens) -> bool {
  if (auto result = parse_expression<T>(remaining)) {
    tokens.emplace_back(std::move(result->first));
    remaining.remove_prefix(result->second);
    offset += result->second;
    return true;
  }

  return false;
}

template <typename... Ts>
static auto try_parse_any(std::string_view &remaining, std::size_t &offset,
                          std::vector<URITemplateToken> &tokens) -> bool {
  return (try_parse<Ts>(remaining, offset, tokens) || ...);
}

URITemplate::URITemplate(const std::string_view source) {
  std::string_view remaining{source};
  std::size_t offset = 0;

  while (!remaining.empty()) {
    try {
      if (!try_parse_any<URITemplateTokenReservedExpansion,
                         URITemplateTokenFragmentExpansion,
                         URITemplateTokenLabelExpansion,
                         URITemplateTokenPathExpansion,
                         URITemplateTokenPathParameterExpansion,
                         URITemplateTokenQueryExpansion,
                         URITemplateTokenQueryContinuationExpansion,
                         URITemplateTokenVariable, URITemplateTokenLiteral>(
              remaining, offset, this->tokens_)) {
        break;
      }
    } catch (URITemplateParseError &error) {
      throw URITemplateParseError(offset + error.column());
    }
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
    const std::function<URITemplateValue(std::string_view name)> &callback)
    const -> std::string {
  std::string result;

  for (const auto &token : this->tokens_) {
    std::visit(
        [&result, &callback](const auto &expansion) {
          using T = std::decay_t<decltype(expansion)>;
          if constexpr (std::is_same_v<T, URITemplateTokenLiteral>) {
            result += expansion.value;
          } else {
            expand_expression<T>(result, expansion.variables, callback);
          }
        },
        token);
  }

  return result;
}

} // namespace sourcemeta::core
