#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonpointer_position.h>

#include <cassert>  // assert
#include <cstddef>  // std::size_t
#include <cstdint>  // std::uint64_t
#include <optional> // std::optional

namespace sourcemeta::core {

auto PointerPositionTracker::operator()(const JSON::ParsePhase phase,
                                        const JSON::Type,
                                        const std::uint64_t line,
                                        const std::uint64_t column,
                                        const JSON &value) -> void {
  if (phase == JSON::ParsePhase::Pre) {
    this->stack.emplace(line, column);
    if (value.is_string()) {
      this->current.push_back(value.to_string());
    } else if (value.is_integer()) {
      this->current.push_back(
          static_cast<Pointer::Token::Index>(value.to_integer()));
    }
  } else if (phase == JSON::ParsePhase::Post) {
    assert(!this->stack.empty());
    this->data.emplace(this->current,
                       Position{this->stack.top().first,
                                this->stack.top().second, line, column});
    this->stack.pop();
    if (!this->current.empty()) {
      this->current.pop_back();
    }
  }
}

auto PointerPositionTracker::get(const Pointer &pointer) const
    -> std::optional<Position> {
  const auto result{this->data.find(pointer)};
  return result == this->data.cend() ? std::nullopt
                                     : std::optional<Position>{result->second};
}

auto PointerPositionTracker::size() const -> std::size_t {
  return this->data.size();
}

} // namespace sourcemeta::core
