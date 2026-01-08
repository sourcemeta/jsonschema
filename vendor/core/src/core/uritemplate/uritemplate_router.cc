#include <sourcemeta/core/uritemplate.h>

#include "helpers.h"

#include <algorithm> // std::ranges::lower_bound
#include <cassert>   // assert
#include <limits>    // std::numeric_limits

namespace sourcemeta::core {

namespace {

using Node = URITemplateRouter::Node;
using NodeType = URITemplateRouter::NodeType;

constexpr auto node_value =
    [](const std::unique_ptr<Node> &child) -> decltype(auto) {
  return child->value;
};

auto find_literal_child(const std::vector<std::unique_ptr<Node>> &literals,
                        const std::string_view segment) -> Node * {
  const auto iterator =
      std::ranges::lower_bound(literals, segment, {}, node_value);
  if (iterator != literals.end() && (*iterator)->value == segment) {
    return iterator->get();
  }
  return nullptr;
}

auto find_or_create_literal_child(std::vector<std::unique_ptr<Node>> &literals,
                                  const std::string_view value) -> Node & {
  auto iterator = std::ranges::lower_bound(literals, value, {}, node_value);
  if (iterator != literals.end() && (*iterator)->value == value) {
    return **iterator;
  }

  auto child = std::make_unique<Node>();
  child->type = NodeType::Literal;
  child->value = value;
  auto &result = *child;
  literals.insert(iterator, std::move(child));
  return result;
}

auto find_or_create_variable_child(std::unique_ptr<Node> &variable,
                                   const std::string_view name,
                                   const NodeType type) -> Node * {
  if (!variable) {
    variable = std::make_unique<Node>();
    variable->type = type;
    variable->value = name;
    return variable.get();
  }

  if (variable->value != name) {
    throw URITemplateRouterVariableMismatchError{variable->value, name};
  }

  if (type == NodeType::Expansion) {
    if (variable->type == NodeType::Variable) {
      variable->type = NodeType::Expansion;
      return variable.get();
    }
  } else if (variable->type == NodeType::Expansion) {
    return nullptr;
  }

  return variable.get();
}

// Find the end of a brace expression (including the closing brace)
inline auto find_expression_end(const char *start, const char *end) -> const
    char * {
  const char *position = start + 1;
  while (position < end && *position != '}') {
    ++position;
  }
  if (position < end) {
    ++position; // include the '}'
  }
  return position;
}

// Extract the current segment (from segment start to next / or end)
inline auto extract_segment(const char *start, const char *end)
    -> std::string_view {
  const char *position = start;
  while (position < end && *position != '/') {
    ++position;
  }
  return {start, static_cast<std::size_t>(position - start)};
}

} // namespace

auto URITemplateRouter::add(const std::string_view uri_template,
                            const Identifier identifier) -> void {
  assert(identifier > 0);

  if (uri_template.empty()) {
    this->root_.identifier = identifier;
    return;
  }

  Node *current = nullptr;
  bool absorbed = false;
  const char *position = uri_template.data();
  const char *const end = position + uri_template.size();

  while (position < end && !absorbed) {
    while (position < end && *position == '/') {
      ++position;
    }

    if (position >= end) {
      break;
    }

    const char *segment_start = position;

    if (*position == '}') {
      throw URITemplateRouterInvalidSegmentError{
          "Unmatched closing brace", extract_segment(segment_start, end)};
    }

    if (*position == '{') {
      const char *expression_start = position;
      const char *expression_end = find_expression_end(position, end);
      std::string_view expression{
          expression_start,
          static_cast<std::size_t>(expression_end - expression_start)};

      ++position;

      if (position >= end) {
        throw URITemplateRouterInvalidSegmentError{"Unclosed brace",
                                                   expression};
      }

      NodeType type = NodeType::Variable;
      if (*position == '+') {
        type = NodeType::Expansion;
        ++position;
        if (position >= end || *position == '}') {
          throw URITemplateRouterInvalidSegmentError{"Empty variable name",
                                                     expression};
        }
      } else if (is_operator(*position) && *position != '+') {
        throw URITemplateRouterInvalidSegmentError{
            "Unsupported URI Template operator", expression};
      } else if (is_reserved_operator(*position)) {
        throw URITemplateRouterInvalidSegmentError{
            "Reserved URI Template operator", expression};
      } else if (*position == '{') {
        throw URITemplateRouterInvalidSegmentError{
            "Nested opening brace", extract_segment(expression_start, end)};
      } else if (*position == ' ') {
        throw URITemplateRouterInvalidSegmentError{"Space before variable name",
                                                   expression};
      } else if (*position == '}') {
        throw URITemplateRouterInvalidSegmentError{"Empty variable name",
                                                   expression};
      }

      const char *varname_start = position;
      while (position < end && *position != '}' && *position != ' ' &&
             !is_modifier(*position) && *position != ',') {
        if (!is_varname_char(*position)) {
          throw URITemplateRouterInvalidSegmentError{
              "Invalid character in variable name", expression};
        }
        ++position;
      }

      if (position >= end) {
        throw URITemplateRouterInvalidSegmentError{"Unclosed brace",
                                                   expression};
      }

      if (*position == ' ') {
        throw URITemplateRouterInvalidSegmentError{
            "Space in variable expression", expression};
      }

      if (*position == ':') {
        throw URITemplateRouterInvalidSegmentError{
            "Prefix modifier not supported", expression};
      }

      if (*position == '*') {
        throw URITemplateRouterInvalidSegmentError{
            "Explode modifier not supported", expression};
      }

      if (*position == ',') {
        throw URITemplateRouterInvalidSegmentError{
            "Multiple variables not supported", expression};
      }

      const std::string_view varname{
          varname_start, static_cast<std::size_t>(position - varname_start)};

      ++position; // skip '}'

      if (position < end && *position != '/') {
        throw URITemplateRouterInvalidSegmentError{
            "Path segment cannot mix literals and variables",
            extract_segment(expression_start, end)};
      }

      if (type == NodeType::Expansion && position < end) {
        throw URITemplateRouterInvalidSegmentError{
            "Reserved expansion must be the last segment", expression};
      }

      auto &variable = current ? current->variable : this->root_.variable;
      auto *result = find_or_create_variable_child(variable, varname, type);
      if (result == nullptr) {
        absorbed = true;
      } else {
        current = result;
      }
    } else {
      while (position < end && *position != '/' && *position != '{') {
        if (*position == '}') {
          throw URITemplateRouterInvalidSegmentError{
              "Unmatched closing brace", extract_segment(segment_start, end)};
        }
        ++position;
      }

      if (position < end && *position == '{') {
        const char *expr_end = find_expression_end(position, end);
        const char *seg_end = expr_end;
        while (seg_end < end && *seg_end != '/') {
          ++seg_end;
        }
        throw URITemplateRouterInvalidSegmentError{
            "Path segment cannot mix literals and variables",
            std::string_view{segment_start, static_cast<std::size_t>(
                                                seg_end - segment_start)}};
      }

      const std::string_view segment{
          segment_start, static_cast<std::size_t>(position - segment_start)};

      auto &literals = current ? current->literals : this->root_.literals;
      current = &find_or_create_literal_child(literals, segment);
    }
  }

  if (current == nullptr && uri_template.size() == 1 &&
      uri_template[0] == '/') {
    current = &find_or_create_literal_child(this->root_.literals, "");
  }

  if (!absorbed && current != nullptr) {
    current->identifier = identifier;
  }
}

auto URITemplateRouter::root() const noexcept -> const Node & {
  return this->root_;
}

auto URITemplateRouter::match(const std::string_view path,
                              const Callback &callback) const -> Identifier {
  if (path.empty()) {
    return this->root_.identifier;
  }

  if (path.size() == 1 && path[0] == '/') {
    if (auto *child = find_literal_child(this->root_.literals, "")) {
      return child->identifier;
    }
    return 0;
  }

  const Node *current = nullptr;
  const char *position = path.data();
  const char *const path_end = position + path.size();

  const std::vector<std::unique_ptr<Node>> *literal_children =
      &this->root_.literals;
  const std::unique_ptr<Node> *variable_child = &this->root_.variable;

  std::size_t variable_index = 0;

  // Skip leading slash
  if (position < path_end && *position == '/') {
    ++position;
  }

  while (true) {
    const char *segment_start = position;
    while (position < path_end && *position != '/') {
      ++position;
    }
    const std::string_view segment{
        segment_start, static_cast<std::size_t>(position - segment_start)};

    // Empty segment (from double slash or trailing slash) doesn't match
    if (segment.empty()) {
      return 0;
    }

    if (auto *literal_match = find_literal_child(*literal_children, segment)) {
      current = literal_match;
    } else if (*variable_child) {
      assert(variable_index <=
             std::numeric_limits<URITemplateRouter::Index>::max());
      if ((*variable_child)->type == NodeType::Expansion) {
        const std::string_view remaining{
            segment_start, static_cast<std::size_t>(path_end - segment_start)};
        callback(static_cast<URITemplateRouter::Index>(variable_index),
                 (*variable_child)->value, remaining);
        return (*variable_child)->identifier;
      }
      callback(static_cast<URITemplateRouter::Index>(variable_index),
               (*variable_child)->value, segment);
      ++variable_index;
      current = variable_child->get();
    } else {
      return 0;
    }

    literal_children = &current->literals;
    variable_child = &current->variable;

    // Check if there's more path
    if (position >= path_end) {
      break;
    }

    // Skip the slash and continue to next segment
    ++position;
  }

  return current ? current->identifier : this->root_.identifier;
}

} // namespace sourcemeta::core
