#include <sourcemeta/jsontoolkit/evaluator_context.h>
#include <sourcemeta/jsontoolkit/evaluator_error.h>

#include <cassert> // assert

namespace sourcemeta::jsontoolkit {

auto EvaluationContext::prepare(const JSON &instance) -> void {
  // Do a full reset for the next run
  assert(this->evaluate_path_.empty());
  assert(this->instance_location_.empty());
  assert(this->frame_sizes.empty());
  assert(this->resources_.empty());
  this->instances_.clear();
  this->instances_.emplace_back(instance);
  this->annotation_blacklist.clear();
  this->annotations_.clear();
  this->labels.clear();
  this->property_as_instance = false;
}

auto EvaluationContext::push_without_traverse(
    const Pointer &relative_schema_location,
    const Pointer &relative_instance_location,
    const std::string &schema_resource, const bool dynamic) -> void {
  // Guard against infinite recursion in a cheap manner, as
  // infinite recursion will manifest itself through huge
  // ever-growing evaluate paths
  constexpr auto EVALUATE_PATH_LIMIT{300};
  if (this->evaluate_path_.size() > EVALUATE_PATH_LIMIT) [[unlikely]] {
    throw sourcemeta::jsontoolkit::SchemaEvaluationError(
        "The evaluation path depth limit was reached "
        "likely due to infinite recursion");
  }

  this->frame_sizes.emplace_back(relative_schema_location.size(),
                                 relative_instance_location.size());
  this->evaluate_path_.push_back(relative_schema_location);
  this->instance_location_.push_back(relative_instance_location);

  if (dynamic) {
    // Note that we are potentially repeatedly pushing back the
    // same schema resource over and over again. However, the
    // logic for making sure this list is "pure" takes a lot of
    // computation power. Being silly seems faster.
    this->resources_.push_back(schema_resource);
  }
}

auto EvaluationContext::push(const Pointer &relative_schema_location,
                             const Pointer &relative_instance_location,
                             const std::string &schema_resource,
                             const bool dynamic) -> void {
  this->push_without_traverse(relative_schema_location,
                              relative_instance_location, schema_resource,
                              dynamic);
  if (!relative_instance_location.empty()) {
    assert(!this->instances_.empty());
    this->instances_.emplace_back(
        get(this->instances_.back().get(), relative_instance_location));
  }
}

auto EvaluationContext::push(const Pointer &relative_schema_location,
                             const Pointer &relative_instance_location,
                             const std::string &schema_resource,
                             const bool dynamic,
                             std::reference_wrapper<const JSON> &&new_instance)
    -> void {
  this->push_without_traverse(relative_schema_location,
                              relative_instance_location, schema_resource,
                              dynamic);
  assert(!relative_instance_location.empty());
  this->instances_.emplace_back(std::move(new_instance));
}

auto EvaluationContext::pop(const bool dynamic) -> void {
  assert(!this->frame_sizes.empty());
  const auto &sizes{this->frame_sizes.back()};
  this->evaluate_path_.pop_back(sizes.first);
  this->instance_location_.pop_back(sizes.second);
  if (sizes.second > 0) {
    this->instances_.pop_back();
  }

  this->frame_sizes.pop_back();

  // TODO: Do schema resource management using hashes to avoid
  // expensive string comparisons
  if (dynamic) {
    assert(!this->resources_.empty());
    this->resources_.pop_back();
  }
}

auto EvaluationContext::annotate(const WeakPointer &current_instance_location,
                                 const JSON &value)
    -> std::pair<std::reference_wrapper<const JSON>, bool> {
  const auto result{this->annotations_.insert({current_instance_location, {}})
                        .first->second.insert({this->evaluate_path(), {}})
                        .first->second.insert(value)};
  return {*(result.first), result.second};
}

auto EvaluationContext::annotations(
    const WeakPointer &current_instance_location,
    const WeakPointer &schema_location) const -> const std::set<JSON> & {
  static const decltype(this->annotations_)::mapped_type::mapped_type
      placeholder;
  // Use `.find()` instead of `.contains()` and `.at()` for performance
  // reasons
  const auto instance_location_result{
      this->annotations_.find(current_instance_location)};
  if (instance_location_result == this->annotations_.end()) {
    return placeholder;
  }

  const auto schema_location_result{
      instance_location_result->second.find(schema_location)};
  if (schema_location_result == instance_location_result->second.end()) {
    return placeholder;
  }

  return schema_location_result->second;
}

auto EvaluationContext::annotations(
    const WeakPointer &current_instance_location) const
    -> const std::map<WeakPointer, std::set<JSON>> & {
  static const decltype(this->annotations_)::mapped_type placeholder;
  // Use `.find()` instead of `.contains()` and `.at()` for performance
  // reasons
  const auto instance_location_result{
      this->annotations_.find(current_instance_location)};
  if (instance_location_result == this->annotations_.end()) {
    return placeholder;
  }

  return instance_location_result->second;
}

auto EvaluationContext::defines_any_adjacent_annotation(
    const WeakPointer &expected_instance_location,
    const WeakPointer &base_evaluate_path, const std::string &keyword) const
    -> bool {
  // TODO: We should be taking masks into account
  // TODO: How can we avoid this expensive pointer manipulation?
  auto expected_evaluate_path{base_evaluate_path};
  expected_evaluate_path.push_back({keyword});
  return !this->annotations(expected_instance_location, expected_evaluate_path)
              .empty();
}

auto EvaluationContext::defines_any_adjacent_annotation(
    const WeakPointer &expected_instance_location,
    const WeakPointer &base_evaluate_path,
    const std::vector<std::string> &keywords) const -> bool {
  for (const auto &keyword : keywords) {
    if (this->defines_any_adjacent_annotation(expected_instance_location,
                                              base_evaluate_path, keyword)) {
      return true;
    }
  }

  return false;
}

auto EvaluationContext::defines_annotation(
    const WeakPointer &expected_instance_location,
    const WeakPointer &base_evaluate_path,
    const std::vector<std::string> &keywords, const JSON &value) const -> bool {
  if (keywords.empty()) {
    return false;
  }

  const auto instance_annotations{
      this->annotations(expected_instance_location)};
  for (const auto &[schema_location, schema_annotations] :
       instance_annotations) {
    assert(!schema_location.empty());
    const auto &keyword{schema_location.back()};

    if (keyword.is_property() &&
        std::find(keywords.cbegin(), keywords.cend(), keyword.to_property()) !=
            keywords.cend() &&
        schema_annotations.contains(value) &&
        schema_location.initial().starts_with(base_evaluate_path)) {
      bool blacklisted = false;
      for (const auto &masked : this->annotation_blacklist) {
        if (schema_location.starts_with(masked) &&
            !this->evaluate_path_.starts_with(masked)) {
          blacklisted = true;
          break;
        }
      }

      if (!blacklisted) {
        return true;
      }
    }
  }

  return false;
}

auto EvaluationContext::largest_annotation_index(
    const WeakPointer &expected_instance_location,
    const std::vector<std::string> &keywords,
    const std::uint64_t default_value) const -> std::uint64_t {
  // TODO: We should be taking masks into account

  std::uint64_t result{default_value};
  for (const auto &[schema_location, schema_annotations] :
       this->annotations(expected_instance_location)) {
    assert(!schema_location.empty());
    const auto &keyword{schema_location.back()};
    if (!keyword.is_property()) {
      continue;
    }

    if (std::find(keywords.cbegin(), keywords.cend(), keyword.to_property()) ==
        keywords.cend()) {
      continue;
    }

    for (const auto &annotation : schema_annotations) {
      if (annotation.is_integer() && annotation.is_positive()) {
        result = std::max(
            result, static_cast<std::uint64_t>(annotation.to_integer()) + 1);
      }
    }
  }

  return result;
}

auto EvaluationContext::enter(const WeakPointer::Token::Property &property)
    -> void {
  this->instance_location_.push_back(property);
  this->instances_.emplace_back(this->instances_.back().get().at(property));
}

auto EvaluationContext::enter(const WeakPointer::Token::Index &index) -> void {
  this->instance_location_.push_back(index);
  this->instances_.emplace_back(this->instances_.back().get().at(index));
}

auto EvaluationContext::leave() -> void {
  this->instance_location_.pop_back();
  this->instances_.pop_back();
}

auto EvaluationContext::instances() const noexcept
    -> const std::vector<std::reference_wrapper<const JSON>> & {
  return this->instances_;
}

auto EvaluationContext::resources() const noexcept
    -> const std::vector<std::string> & {
  return this->resources_;
}

auto EvaluationContext::evaluate_path() const noexcept -> const WeakPointer & {
  return this->evaluate_path_;
}

auto EvaluationContext::instance_location() const noexcept
    -> const WeakPointer & {
  return this->instance_location_;
}

auto EvaluationContext::target_type(const TargetType type) noexcept -> void {
  this->property_as_instance = (type == TargetType::Key);
}

auto EvaluationContext::resolve_target() -> const JSON & {
  if (this->property_as_instance) [[unlikely]] {
    assert(!this->instance_location().empty());
    assert(this->instance_location().back().is_property());
    // For efficiency, as we likely reference the same JSON values
    // over and over again
    // TODO: Get rid of this once we have weak pointers
    static std::set<JSON> property_values;
    return *(
        property_values.emplace(this->instance_location().back().to_property())
            .first);
  }

  return this->instances_.back().get();
}

auto EvaluationContext::mark(const std::size_t id,
                             const SchemaCompilerTemplate &children) -> void {
  this->labels.try_emplace(id, children);
}

// TODO: At least currently, we only need to mask if a schema
// makes use of `unevaluatedProperties` or `unevaluatedItems`
// Detect if a schema does need this so if not, we avoid
// an unnecessary copy
auto EvaluationContext::mask() -> void {
  this->annotation_blacklist.push_back(this->evaluate_path_);
}

auto EvaluationContext::jump(const std::size_t id) const noexcept
    -> const SchemaCompilerTemplate & {
  assert(this->labels.contains(id));
  return this->labels.at(id).get();
}

auto EvaluationContext::find_dynamic_anchor(const std::string &anchor) const
    -> std::optional<std::size_t> {
  for (const auto &resource : this->resources()) {
    std::ostringstream name;
    name << resource;
    name << '#';
    name << anchor;
    const auto label{std::hash<std::string>{}(name.str())};
    if (this->labels.contains(label)) {
      return label;
    }
  }

  return std::nullopt;
}

} // namespace sourcemeta::jsontoolkit
