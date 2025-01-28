#include <sourcemeta/jsontoolkit/jsonpointer_position.h>

#include <cassert> // assert

namespace sourcemeta::jsontoolkit {

auto PositionTracker::operator()(const CallbackPhase phase, const JSON::Type,
                                 const std::uint64_t line,
                                 const std::uint64_t column, const JSON &value)
    -> void {
  if (phase == CallbackPhase::Pre) {
    this->stack.push({line, column});
    if (value.is_string()) {
      this->current.push_back(value.to_string());
    } else if (value.is_integer()) {
      this->current.push_back(
          static_cast<Pointer::Token::Index>(value.to_integer()));
    }
  } else if (phase == CallbackPhase::Post) {
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

auto PositionTracker::get(const Pointer &pointer) const
    -> std::optional<Position> {
  const auto result{this->data.find(pointer)};
  return result == this->data.cend() ? std::nullopt
                                     : std::optional<Position>{result->second};
}

auto PositionTracker::size() const -> std::size_t { return this->data.size(); }

} // namespace sourcemeta::jsontoolkit
