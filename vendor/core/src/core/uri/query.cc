#include <sourcemeta/core/uri.h>

#include <cstddef>     // std::size_t, std::string_view::npos
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <utility>     // std::pair

namespace {

auto parse_pair(const std::string_view raw, const std::size_t pair_start)
    -> std::pair<std::string_view, std::string_view> {
  const auto separator{raw.find('&', pair_start)};
  const auto pair_end{separator == std::string_view::npos ? raw.size()
                                                          : separator};
  const auto pair_view{raw.substr(pair_start, pair_end - pair_start)};
  const auto equals_index{pair_view.find('=')};
  if (equals_index == std::string_view::npos) {
    return {pair_view, std::string_view{}};
  }
  return {pair_view.substr(0, equals_index),
          pair_view.substr(equals_index + 1)};
}

} // namespace

namespace sourcemeta::core {

URI::Query::Query(const std::string_view raw) : raw_{raw} {}

auto URI::Query::raw() const -> std::string_view { return this->raw_; }

auto URI::query() const -> std::optional<URI::Query> {
  if (!this->query_.has_value()) {
    return std::nullopt;
  }
  return URI::Query{this->query_.value()};
}

URI::Query::const_iterator::const_iterator(const std::string_view raw,
                                           const std::size_t pair_start)
    : raw_{raw}, pair_start_{pair_start} {
  if (this->pair_start_ != std::string_view::npos) {
    this->current_ = parse_pair(this->raw_, this->pair_start_);
  }
}

auto URI::Query::const_iterator::operator*() const -> reference {
  return this->current_;
}

auto URI::Query::const_iterator::operator->() const -> pointer {
  return &this->current_;
}

auto URI::Query::const_iterator::operator++() -> const_iterator & {
  const auto separator{this->raw_.find('&', this->pair_start_)};
  if (separator == std::string_view::npos) {
    this->pair_start_ = std::string_view::npos;
  } else {
    this->pair_start_ = separator + 1;
    this->current_ = parse_pair(this->raw_, this->pair_start_);
  }
  return *this;
}

auto URI::Query::const_iterator::operator++(int) -> const_iterator {
  auto previous{*this};
  ++(*this);
  return previous;
}

auto URI::Query::const_iterator::operator==(const const_iterator &other) const
    -> bool {
  return this->pair_start_ == other.pair_start_;
}

auto URI::Query::const_iterator::operator!=(const const_iterator &other) const
    -> bool {
  return !(*this == other);
}

auto URI::Query::begin() const -> const_iterator {
  if (this->raw_.empty()) {
    return this->end();
  }
  return const_iterator{this->raw_, 0};
}

auto URI::Query::end() const -> const_iterator {
  return const_iterator{this->raw_, std::string_view::npos};
}

// First-wins on duplicates, matching WHATWG `URLSearchParams.get()`
// and most major URL libraries
auto URI::Query::at(const std::string_view name) const
    -> std::optional<std::string_view> {
  for (const auto &pair : *this) {
    if (pair.first == name) {
      return pair.second;
    }
  }
  return std::nullopt;
}

} // namespace sourcemeta::core
