#include <sourcemeta/core/regex.h>
#include <sourcemeta/core/uritemplate.h>

#include "helpers.h"

#include <algorithm> // std::ranges::lower_bound, std::ranges::find_if
#include <cassert>   // assert
#include <limits>    // std::numeric_limits
#include <tuple>     // std::get, std::make_tuple

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

inline auto is_expansion_type(const NodeType type) noexcept -> bool {
  return type == NodeType::Expansion || type == NodeType::OptionalExpansion;
}

auto find_or_create_variable_child(std::unique_ptr<Node> &variable,
                                   const std::string_view name,
                                   const NodeType type,
                                   const std::string_view expression)
    -> Node * {
  if (!variable) {
    variable = std::make_unique<Node>();
    variable->type = type;
    variable->value = name;
    return variable.get();
  }

  if (variable->value != name) {
    throw URITemplateRouterVariableMismatchError{variable->value, name};
  }

  if (is_expansion_type(variable->type) && is_expansion_type(type) &&
      variable->type != type) {
    throw URITemplateRouterInvalidSegmentError{
        "Conflicting expansion operators on the same path position",
        expression};
  }

  if (is_expansion_type(type)) {
    if (variable->type == NodeType::Variable) {
      variable->type = type;
      return variable.get();
    }
  } else if (is_expansion_type(variable->type)) {
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

inline auto finalize_match(const Node &otherwise,
                           const URITemplateRouter::Identifier identifier,
                           const URITemplateRouter::Identifier context)
    -> std::pair<URITemplateRouter::Identifier, URITemplateRouter::Identifier> {
  if (identifier == 0) {
    return {URITemplateRouter::Identifier{0}, otherwise.context};
  }

  return {identifier, context};
}

} // namespace

URITemplateRouter::URITemplateRouter(const std::string_view base_path,
                                     const std::string_view base_url)
    : base_path_{base_path}, base_url_{base_url} {
  assert(this->base_path_.empty() || this->base_path_.front() == '/');
  const auto base_path_last = this->base_path_.find_last_not_of('/');
  if (base_path_last == std::string::npos) {
    this->base_path_.clear();
  } else {
    this->base_path_.erase(base_path_last + 1);
  }

  const auto base_url_last = this->base_url_.find_last_not_of('/');
  if (base_url_last == std::string::npos) {
    this->base_url_.clear();
  } else {
    this->base_url_.erase(base_url_last + 1);
  }
}

auto URITemplateRouter::base_path() const noexcept -> std::string_view {
  return this->base_path_;
}

auto URITemplateRouter::base_url() const noexcept -> std::string_view {
  return this->base_url_;
}

auto URITemplateRouter::size() const noexcept -> std::size_t {
  return this->entries_.size();
}

auto URITemplateRouter::at(const std::size_t index) const -> Identifier {
  assert(index < this->entries_.size());
  return std::get<0>(this->entries_[index]);
}

auto URITemplateRouter::context(const Identifier identifier) const
    -> Identifier {
  assert(identifier > 0);
  const auto entry = std::ranges::find_if(
      this->entries_, [&identifier](const auto &candidate) {
        return std::get<0>(candidate) == identifier;
      });
  assert(entry != this->entries_.end());
  return std::get<1>(*entry);
}

auto URITemplateRouter::path(const Identifier identifier) const -> std::string {
  assert(identifier > 0);
  const auto entry = std::ranges::find_if(
      this->entries_, [&identifier](const auto &candidate) {
        return std::get<0>(candidate) == identifier;
      });
  assert(entry != this->entries_.end());
  return std::string{std::get<2>(*entry)};
}

auto URITemplateRouter::operation(const std::string_view operation_id) const
    -> std::pair<Identifier, Identifier> {
  const auto iterator = this->operations_.find(operation_id);
  if (iterator == this->operations_.end()) {
    return {Identifier{0}, Identifier{0}};
  }
  return iterator->second;
}

auto URITemplateRouter::operation_id(const Identifier identifier) const
    -> std::string_view {
  if (identifier == 0) {
    return {};
  }
  const auto entry =
      std::ranges::find_if(this->operations_, [&identifier](const auto &item) {
        return item.second.first == identifier;
      });
  if (entry == this->operations_.end()) {
    return {};
  }
  return entry->first;
}

auto URITemplateRouter::otherwise(const Identifier context,
                                  const std::span<const Argument> arguments)
    -> void {
  this->otherwise_.context = context;

  const auto existing = std::ranges::find_if(
      this->arguments_, [](const auto &entry) { return entry.first == 0; });
  if (existing == this->arguments_.end()) {
    if (!arguments.empty()) {
      this->arguments_.emplace_back(
          Identifier{0},
          std::vector<Argument>{arguments.begin(), arguments.end()});
    }
  } else {
    if (arguments.empty()) {
      this->arguments_.erase(existing);
    } else {
      existing->second.assign(arguments.begin(), arguments.end());
    }
  }
}

auto URITemplateRouter::add(const std::string_view uri_template,
                            const std::string_view operation_id,
                            const Identifier identifier,
                            const Identifier context,
                            const std::span<const Argument> arguments) -> void {
  assert(identifier > 0);

  static const Regex operation_id_regex =
      to_regex("^[a-zA-Z][a-zA-Z0-9_-]{0,63}$").value();

  if (!matches(operation_id_regex, operation_id)) {
    throw URITemplateRouterInvalidOperationIdError{operation_id};
  }

  if (this->operations_.contains(operation_id)) {
    throw URITemplateRouterDuplicateOperationIdError{operation_id};
  }

  // Walk base path segments to establish the trie prefix
  Node *current = nullptr;
  if (!this->base_path_.empty()) {
    const char *base_position = this->base_path_.data();
    const char *const base_end = base_position + this->base_path_.size();
    while (base_position < base_end) {
      while (base_position < base_end && *base_position == '/') {
        ++base_position;
      }
      if (base_position >= base_end) {
        break;
      }
      const char *segment_start = base_position;
      while (base_position < base_end && *base_position != '/') {
        ++base_position;
      }
      const std::string_view segment{
          segment_start,
          static_cast<std::size_t>(base_position - segment_start)};
      auto &literals = current ? current->literals : this->root_.literals;
      current = &find_or_create_literal_child(literals, segment);
    }
  }

  if (uri_template.empty()) {
    auto &target = current ? *current : this->root_;
    const auto previous_identifier = target.identifier;
    if (previous_identifier == 0) {
      this->entries_.emplace_back(identifier, context, uri_template);
    } else {
      const auto existing = std::ranges::find_if(
          this->entries_, [&previous_identifier](const auto &candidate) {
            return std::get<0>(candidate) == previous_identifier;
          });
      if (existing != this->entries_.end()) {
        *existing = std::make_tuple(identifier, context, uri_template);
      }
      std::erase_if(this->operations_,
                    [&previous_identifier](const auto &entry) {
                      return entry.second.first == previous_identifier;
                    });
    }
    target.identifier = identifier;
    target.context = context;
    this->operations_.emplace(
        operation_id, std::pair<Identifier, Identifier>{identifier, context});
    if (!arguments.empty()) {
      assert(std::ranges::none_of(this->arguments_,
                                  [&identifier](const auto &entry) {
                                    return entry.first == identifier;
                                  }));
      this->arguments_.emplace_back(
          identifier,
          std::vector<Argument>{arguments.begin(), arguments.end()});
    }

    return;
  }

  Node *base_path_end = current;
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
      bool path_segment_operator = false;
      if (*position == '+') {
        type = NodeType::Expansion;
        ++position;
        if (position >= end || *position == '}') {
          throw URITemplateRouterInvalidSegmentError{"Empty variable name",
                                                     expression};
        }
      } else if (*position == '/') {
        path_segment_operator = true;
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

      const char *const varname_end = position;

      if (*position == ' ') {
        throw URITemplateRouterInvalidSegmentError{
            "Space in variable expression", expression};
      }

      if (*position == ':') {
        throw URITemplateRouterInvalidSegmentError{
            "Prefix modifier not supported", expression};
      }

      if (*position == '*') {
        if (!path_segment_operator) {
          throw URITemplateRouterInvalidSegmentError{
              "Explode modifier not supported", expression};
        }
        if (varname_end == varname_start) {
          throw URITemplateRouterInvalidSegmentError{"Empty variable name",
                                                     expression};
        }
        type = NodeType::OptionalExpansion;
        ++position;
        if (position >= end || *position != '}') {
          throw URITemplateRouterInvalidSegmentError{
              "Unexpected characters after explode modifier", expression};
        }
      } else if (path_segment_operator) {
        throw URITemplateRouterInvalidSegmentError{
            "Path-segment expansion without the explode modifier is not "
            "supported",
            expression};
      }

      if (*position == ',') {
        throw URITemplateRouterInvalidSegmentError{
            "Multiple variables not supported", expression};
      }

      const std::string_view varname{
          varname_start, static_cast<std::size_t>(varname_end - varname_start)};

      ++position; // skip '}'

      if (position < end && *position != '/') {
        if (*position != '{' || position + 1 >= end || *(position + 1) != '/') {
          throw URITemplateRouterInvalidSegmentError{
              "Path segment cannot mix literals and variables",
              extract_segment(expression_start, end)};
        }
      }

      if (is_expansion_type(type) && position < end) {
        throw URITemplateRouterInvalidSegmentError{
            "Expansion operator must be the last segment", expression};
      }

      auto &variable = current ? current->variable : this->root_.variable;
      auto *result =
          find_or_create_variable_child(variable, varname, type, expression);
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
        if (position + 1 < end && *(position + 1) == '/') {
          const std::string_view segment{
              segment_start,
              static_cast<std::size_t>(position - segment_start)};
          auto &literals = current ? current->literals : this->root_.literals;
          current = &find_or_create_literal_child(literals, segment);
          continue;
        }
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

  if (current == base_path_end && uri_template.size() == 1 &&
      uri_template[0] == '/') {
    auto &literals = current ? current->literals : this->root_.literals;
    current = &find_or_create_literal_child(literals, "");
  }

  if (!absorbed && current != nullptr) {
    const auto previous_identifier = current->identifier;
    if (previous_identifier == 0) {
      this->entries_.emplace_back(identifier, context, uri_template);
    } else {
      const auto existing = std::ranges::find_if(
          this->entries_, [&previous_identifier](const auto &candidate) {
            return std::get<0>(candidate) == previous_identifier;
          });
      if (existing != this->entries_.end()) {
        *existing = std::make_tuple(identifier, context, uri_template);
      }
      std::erase_if(this->operations_,
                    [&previous_identifier](const auto &entry) {
                      return entry.second.first == previous_identifier;
                    });
    }
    current->identifier = identifier;
    current->context = context;
    this->operations_.emplace(
        operation_id, std::pair<Identifier, Identifier>{identifier, context});
    if (!arguments.empty()) {
      assert(std::ranges::none_of(this->arguments_,
                                  [&identifier](const auto &entry) {
                                    return entry.first == identifier;
                                  }));
      this->arguments_.emplace_back(
          identifier,
          std::vector<Argument>{arguments.begin(), arguments.end()});
    }
  }
}

auto URITemplateRouter::root() const noexcept -> const Node & {
  return this->root_;
}

auto URITemplateRouter::arguments(const Identifier identifier,
                                  const ArgumentCallback &callback) const
    -> void {
  for (const auto &entry : this->arguments_) {
    if (entry.first == identifier) {
      for (const auto &argument : entry.second) {
        callback(argument.first, argument.second);
      }

      return;
    }
  }
}

auto URITemplateRouter::arguments() const noexcept
    -> const std::vector<std::pair<Identifier, std::vector<Argument>>> & {
  return this->arguments_;
}

auto URITemplateRouter::match(const std::string_view path,
                              const Callback &callback) const
    -> std::pair<Identifier, Identifier> {
  if (path.empty()) {
    return finalize_match(this->otherwise_, this->root_.identifier,
                          this->root_.context);
  }

  if (path.size() == 1 && path[0] == '/') {
    if (auto *child = find_literal_child(this->root_.literals, "")) {
      return finalize_match(this->otherwise_, child->identifier,
                            child->context);
    }
    return finalize_match(this->otherwise_, 0, 0);
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
      return finalize_match(this->otherwise_, 0, 0);
    }

    if (auto *literal_match = find_literal_child(*literal_children, segment)) {
      current = literal_match;
    } else if (*variable_child) {
      assert(variable_index <=
             std::numeric_limits<URITemplateRouter::Index>::max());
      if (is_expansion_type((*variable_child)->type)) {
        const std::string_view remaining{
            segment_start, static_cast<std::size_t>(path_end - segment_start)};
        callback(static_cast<URITemplateRouter::Index>(variable_index),
                 (*variable_child)->value, remaining);
        return finalize_match(this->otherwise_, (*variable_child)->identifier,
                              (*variable_child)->context);
      }
      callback(static_cast<URITemplateRouter::Index>(variable_index),
               (*variable_child)->value, segment);
      ++variable_index;
      current = variable_child->get();
    } else {
      return finalize_match(this->otherwise_, 0, 0);
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

  if (current && current->identifier == 0 && current->variable &&
      current->variable->type == NodeType::OptionalExpansion) {
    assert(variable_index <=
           std::numeric_limits<URITemplateRouter::Index>::max());
    callback(static_cast<URITemplateRouter::Index>(variable_index),
             current->variable->value, std::string_view{});
    return finalize_match(this->otherwise_, current->variable->identifier,
                          current->variable->context);
  }

  return current ? finalize_match(this->otherwise_, current->identifier,
                                  current->context)
                 : finalize_match(this->otherwise_, this->root_.identifier,
                                  this->root_.context);
}

} // namespace sourcemeta::core
