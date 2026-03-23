#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonpointer.h>

#include <algorithm>   // std::count_if
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto PointerPositionTracker::ensure_index() const -> void {
  if (this->indexed) {
    return;
  }

  this->indexed = true;
  this->trie.push_back({.position = std::nullopt,
                        .index_children = {},
                        .property_children = {}});

  std::size_t current_node{0};
  std::vector<std::pair<std::size_t, std::pair<std::uint64_t, std::uint64_t>>>
      node_stack;

  for (const auto &event : this->events) {
    switch (event.phase) {
      case JSON::ParsePhase::Pre:
        node_stack.emplace_back(current_node,
                                std::make_pair(event.line, event.column));

        switch (event.context) {
          case JSON::ParseContext::Property: {
            assert(event.property != nullptr);
            const std::string_view key{*event.property};
            auto iterator{this->trie[current_node].property_children.find(key)};
            if (iterator == this->trie[current_node].property_children.end()) {
              const auto node_index{this->trie.size()};
              this->trie.push_back({.position = std::nullopt,
                                    .index_children = {},
                                    .property_children = {}});
              this->trie[current_node].property_children.emplace(key,
                                                                 node_index);
              current_node = node_index;
            } else {
              current_node = iterator->second;
            }
            break;
          }
          case JSON::ParseContext::Index: {
            auto iterator{
                this->trie[current_node].index_children.find(event.index)};
            if (iterator == this->trie[current_node].index_children.end()) {
              const auto node_index{this->trie.size()};
              this->trie.push_back({.position = std::nullopt,
                                    .index_children = {},
                                    .property_children = {}});
              this->trie[current_node].index_children.emplace(event.index,
                                                              node_index);
              current_node = node_index;
            } else {
              current_node = iterator->second;
            }
            break;
          }
          case JSON::ParseContext::Root:
            break;
        }

        break;
      case JSON::ParsePhase::Post:
        assert(!node_stack.empty());
        this->trie[current_node].position =
            Position{node_stack.back().second.first,
                     node_stack.back().second.second, event.line, event.column};
        current_node = node_stack.back().first;
        node_stack.pop_back();
        break;
      default:
        assert(false);
        break;
    }
  }
}

auto PointerPositionTracker::operator()(
    const JSON::ParsePhase phase, const JSON::Type, const std::uint64_t line,
    const std::uint64_t column, const JSON::ParseContext context,
    const std::size_t index, const JSON::String &property) -> void {
  this->events.push_back({.phase = phase,
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
      const auto &children{this->trie[node].property_children};
      const auto iterator{children.find(std::string_view{token.to_property()})};
      if (iterator == children.end()) {
        return std::nullopt;
      }
      node = iterator->second;
    } else {
      const auto &children{this->trie[node].index_children};
      const auto iterator{children.find(token.to_index())};
      if (iterator == children.end()) {
        return std::nullopt;
      }
      node = iterator->second;
    }
  }

  return this->trie[node].position;
}

auto PointerPositionTracker::size() const -> std::size_t {
  return static_cast<std::size_t>(std::count_if(
      this->events.cbegin(), this->events.cend(), [](const Event &event) {
        return event.phase == JSON::ParsePhase::Post;
      }));
}

auto PointerPositionTracker::to_json() const -> JSON {
  auto result{JSON::make_object()};
  Pointer current;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> start_stack;

  for (const auto &event : this->events) {
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
      default:
        assert(false);
        break;
    }
  }

  assert(current.empty());
  assert(start_stack.empty());
  return result;
}

} // namespace sourcemeta::core
