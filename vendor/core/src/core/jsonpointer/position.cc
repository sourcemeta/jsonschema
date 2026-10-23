#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/text.h>

#include <algorithm>   // std::count_if
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace {

// A property step and an index step differ in nothing but which of the two
// child maps of a node the step is remembered in, so what a step does with
// what it finds there is spelled out once here. The map is reached through the
// node each time, as taking in a new node may move the ones already held
template <typename Node, typename Children, typename Key>
auto descend_into(std::vector<Node> &trie, Children Node::*children,
                  const Key &key, std::size_t &current_node) -> void {
  const auto match{(trie[current_node].*children).find(key)};
  if (match != (trie[current_node].*children).end()) {
    current_node = match->second;
    return;
  }

  const auto node_index{trie.size()};
  trie.push_back({.position = std::nullopt,
                  .index_children = {},
                  .property_children = {}});
  (trie[current_node].*children).emplace(key, node_index);
  current_node = node_index;
}

} // namespace

namespace sourcemeta::core {

auto PointerPositionTracker::ensure_index() const -> void {
  if (this->indexed_) {
    return;
  }

  this->indexed_ = true;
  this->trie_.push_back({.position = std::nullopt,
                         .index_children = {},
                         .property_children = {}});

  std::size_t current_node{0};
  std::vector<std::pair<std::size_t, std::pair<std::uint64_t, std::uint64_t>>>
      node_stack;

  for (const auto &event : this->events_) {
    switch (event.phase) {
      case JSON::ParsePhase::Pre:
        node_stack.emplace_back(current_node,
                                std::make_pair(event.line, event.column));

        switch (event.context) {
          case JSON::ParseContext::Property:
            assert(event.property != nullptr);
            descend_into(this->trie_, &TrieNode::property_children,
                         std::string_view{*event.property}, current_node);
            break;
          case JSON::ParseContext::Index:
            descend_into(this->trie_, &TrieNode::index_children, event.index,
                         current_node);
            break;
          case JSON::ParseContext::Root:
            break;
        }

        break;
      case JSON::ParsePhase::Post:
        assert(!node_stack.empty());
        this->trie_[current_node].position =
            Position{node_stack.back().second.first,
                     node_stack.back().second.second, event.line, event.column};
        current_node = node_stack.back().first;
        node_stack.pop_back();
        break;
    }
  }
}

auto PointerPositionTracker::operator()(const JSON::ParsePhase phase,
                                        [[maybe_unused]] const JSON::Type type,
                                        const std::uint64_t line,
                                        const std::uint64_t column,
                                        const JSON::ParseContext context,
                                        const std::size_t index,
                                        const JSON::String &property) -> void {
  this->events_.push_back({.phase = phase,
                           .context = context,
                           .index = index,
                           .property = context == JSON::ParseContext::Property
                                           ? &property
                                           : nullptr,
                           .line = line,
                           .column = column});
}

auto PointerPositionTracker::get(const Pointer &pointer) const
    -> std::optional<Position> {
  this->ensure_index();
  std::size_t node{0};
  for (const auto &token : pointer) {
    if (token.is_property()) {
      const auto &children{this->trie_[node].property_children};
      const auto iterator{children.find(std::string_view{token.to_property()})};
      if (iterator == children.end()) {
        return std::nullopt;
      }
      node = iterator->second;
    } else {
      const auto &children{this->trie_[node].index_children};
      const auto iterator{children.find(token.to_index())};
      if (iterator == children.end()) {
        // If the currently referenced value is a JSON object, the new
        // referenced value is the object member with the name identified by the
        // reference token.
        // See https://www.rfc-editor.org/rfc/rfc6901#section-4
        const auto &properties{this->trie_[node].property_children};
        DigitsBuffer buffer;
        const auto property_iterator{
            properties.find(digits_view(token.to_index(), buffer))};
        if (property_iterator == properties.end()) {
          return std::nullopt;
        }
        node = property_iterator->second;
      } else {
        node = iterator->second;
      }
    }
  }

  return this->trie_[node].position;
}

auto PointerPositionTracker::size() const -> std::size_t {
  return static_cast<std::size_t>(
      std::count_if(this->events_.cbegin(), this->events_.cend(),
                    [](const Event &event) -> bool {
                      return event.phase == JSON::ParsePhase::Post;
                    }));
}

auto PointerPositionTracker::to_json() const -> JSON {
  auto result{JSON::make_object()};
  Pointer current;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> start_stack;

  for (const auto &event : this->events_) {
    switch (event.phase) {
      case JSON::ParsePhase::Pre:
        start_stack.emplace_back(event.line, event.column);
        switch (event.context) {
          case JSON::ParseContext::Property:
            assert(event.property != nullptr);
            current.push_back(*event.property);
            break;
          case JSON::ParseContext::Index:
            current.push_back(event.index);
            break;
          default:
            break;
        }

        break;
      case JSON::ParsePhase::Post:
        assert(!start_stack.empty());
        result.assign_assume_new(
            to_string(current),
            sourcemeta::core::to_json(Position{start_stack.back().first,
                                               start_stack.back().second,
                                               event.line, event.column}));
        start_stack.pop_back();
        if (!current.empty()) {
          current.pop_back();
        }

        break;
    }
  }

  assert(current.empty());
  assert(start_stack.empty());
  return result;
}

} // namespace sourcemeta::core
