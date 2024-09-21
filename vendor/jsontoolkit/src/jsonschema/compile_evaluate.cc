#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include "trace.h"

#include <algorithm>   // std::min, std::any_of
#include <cassert>     // assert
#include <functional>  // std::reference_wrapper
#include <iterator>    // std::distance, std::advance
#include <limits>      // std::numeric_limits
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <type_traits> // std::is_same_v
#include <vector>      // std::vector

namespace {

class EvaluationContext {
public:
  using Pointer = sourcemeta::jsontoolkit::WeakPointer;
  using JSON = sourcemeta::jsontoolkit::JSON;
  using Template = sourcemeta::jsontoolkit::SchemaCompilerTemplate;
  EvaluationContext(const JSON &instance) : instances_{instance} {};

  auto annotate(const Pointer &current_instance_location, const JSON &value)
      -> std::pair<std::reference_wrapper<const JSON>, bool> {
    const auto result{this->annotations_.insert({current_instance_location, {}})
                          .first->second.insert({this->evaluate_path(), {}})
                          .first->second.insert(value)};
    return {*(result.first), result.second};
  }

private:
  auto annotations(const Pointer &current_instance_location,
                   const Pointer &schema_location) const -> const auto & {
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

  auto annotations(const Pointer &current_instance_location) const -> const
      auto & {
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

public:
  auto
  defines_any_adjacent_annotation(const Pointer &expected_instance_location,
                                  const Pointer &base_evaluate_path,
                                  const std::string &keyword) const -> bool {
    // TODO: We should be taking masks into account
    // TODO: How can we avoid this expensive pointer manipulation?
    auto expected_evaluate_path{base_evaluate_path};
    expected_evaluate_path.push_back({keyword});
    return !this->annotations(expected_instance_location,
                              expected_evaluate_path)
                .empty();
  }

  auto defines_any_adjacent_annotation(
      const Pointer &expected_instance_location,
      const Pointer &base_evaluate_path,
      const std::vector<std::string> &keywords) const -> bool {
    for (const auto &keyword : keywords) {
      if (this->defines_any_adjacent_annotation(expected_instance_location,
                                                base_evaluate_path, keyword)) {
        return true;
      }
    }

    return false;
  }

  auto defines_annotation(const Pointer &expected_instance_location,
                          const Pointer &base_evaluate_path,
                          const std::vector<std::string> &keywords,
                          const JSON &value) const -> bool {
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
          std::find(keywords.cbegin(), keywords.cend(),
                    keyword.to_property()) != keywords.cend() &&
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

  auto largest_annotation_index(const Pointer &expected_instance_location,
                                const std::vector<std::string> &keywords,
                                const std::uint64_t default_value) const
      -> std::uint64_t {
    // TODO: We should be taking masks into account

    std::uint64_t result{default_value};
    for (const auto &[schema_location, schema_annotations] :
         this->annotations(expected_instance_location)) {
      assert(!schema_location.empty());
      const auto &keyword{schema_location.back()};
      if (!keyword.is_property()) {
        continue;
      }

      if (std::find(keywords.cbegin(), keywords.cend(),
                    keyword.to_property()) == keywords.cend()) {
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

  template <typename T> auto push(const T &step) -> void {
    // Guard against infinite recursion in a cheap manner, as
    // infinite recursion will manifest itself through huge
    // ever-growing evaluate paths
    constexpr auto EVALUATE_PATH_LIMIT{400};
    if (this->evaluate_path_.size() > EVALUATE_PATH_LIMIT) [[unlikely]] {
      throw sourcemeta::jsontoolkit::SchemaEvaluationError(
          "The evaluation path depth limit was reached "
          "likely due to infinite recursion");
    }

    this->frame_sizes.emplace_back(step.relative_schema_location.size(),
                                   step.relative_instance_location.size());
    this->evaluate_path_.push_back(step.relative_schema_location);
    this->instance_location_.push_back(step.relative_instance_location);
    if (!step.relative_instance_location.empty()) {
      this->instances_.emplace_back(
          get(this->instances_.back().get(), step.relative_instance_location));
    }

    if (step.dynamic) {
      // Note that we are potentially repeatedly pushing back the
      // same schema resource over and over again. However, the
      // logic for making sure this list is "pure" takes a lot of
      // computation power. Being silly seems faster.
      this->resources_.push_back(step.schema_resource);
    }
  }

  template <typename T> auto pop(const T &step) -> void {
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
    if (step.dynamic) {
      assert(!this->resources_.empty());
      this->resources_.pop_back();
    }
  }

  auto enter(const Pointer::Token::Property &property) -> void {
    this->instance_location_.push_back(property);
    this->instances_.emplace_back(this->instances_.back().get().at(property));
  }

  auto enter(const Pointer::Token::Index &index) -> void {
    this->instance_location_.push_back(index);
    this->instances_.emplace_back(this->instances_.back().get().at(index));
  }

  auto leave() -> void {
    this->instance_location_.pop_back();
    this->instances_.pop_back();
  }

  auto instances() const noexcept -> const auto & { return this->instances_; }

  auto resources() const noexcept -> const std::vector<std::string> & {
    return this->resources_;
  }

  auto evaluate_path() const noexcept -> const Pointer & {
    return this->evaluate_path_;
  }

  auto instance_location() const noexcept -> const Pointer & {
    return this->instance_location_;
  }

  enum class TargetType { Key, Value };
  auto target_type(const TargetType type) noexcept -> void {
    this->property_as_instance = (type == TargetType::Key);
  }

  auto resolve_target() -> const JSON & {
    if (this->property_as_instance) [[unlikely]] {
      assert(!this->instance_location().empty());
      assert(this->instance_location().back().is_property());
      // For efficiency, as we likely reference the same JSON values
      // over and over again
      // TODO: Get rid of this once we have weak pointers
      static std::set<JSON> property_values;
      return *(property_values
                   .emplace(this->instance_location().back().to_property())
                   .first);
    }

    return this->instances_.back().get();
  }

  auto mark(const std::size_t id, const Template &children) -> void {
    this->labels.try_emplace(id, children);
  }

  // TODO: At least currently, we only need to mask if a schema
  // makes use of `unevaluatedProperties` or `unevaluatedItems`
  // Detect if a schema does need this so if not, we avoid
  // an unnecessary copy
  auto mask() -> void {
    this->annotation_blacklist.push_back(this->evaluate_path_);
  }

  auto jump(const std::size_t id) const noexcept -> const Template & {
    assert(this->labels.contains(id));
    return this->labels.at(id).get();
  }

  auto find_dynamic_anchor(const std::string &anchor) const
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

public:
  const JSON null{nullptr};

private:
  std::vector<std::reference_wrapper<const JSON>> instances_;
  Pointer evaluate_path_;
  Pointer instance_location_;
  std::vector<std::pair<std::size_t, std::size_t>> frame_sizes;
  // TODO: Keep hashes of schema resources URI instead for performance reasons
  std::vector<std::string> resources_;
  std::vector<Pointer> annotation_blacklist;
  // We don't use a pair for holding the two pointers for runtime
  // efficiency when resolving keywords like `unevaluatedProperties`
  std::map<Pointer, std::map<Pointer, std::set<JSON>>> annotations_;
  std::map<std::size_t, const std::reference_wrapper<const Template>> labels;
  bool property_as_instance{false};
};

auto evaluate_step(
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &step,
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode mode,
    const std::optional<
        sourcemeta::jsontoolkit::SchemaCompilerEvaluationCallback> &callback,
    EvaluationContext &context) -> bool {
  SOURCEMETA_TRACE_REGISTER_ID(trace_id);
  using namespace sourcemeta::jsontoolkit;

#define STRINGIFY(x) #x
#define IS_STEP(step_type) std::holds_alternative<step_type>(step)

#define EVALUATE_BEGIN(step_category, step_type, precondition)                 \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  context.push(step_category);                                                 \
  const auto &target{context.resolve_target()};                                \
  if (!(precondition)) {                                                       \
    context.pop(step_category);                                                \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_TARGET(step_category, step_type, precondition)       \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  if (!(precondition)) {                                                       \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  context.push(step_category);                                                 \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION(step_category, step_type)               \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  context.push(step_category);                                                 \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  bool result{false};

#define EVALUATE_END(step_category, step_type)                                 \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Post, result, step,         \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  context.pop(step_category);                                                  \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return result;

  // As a safety guard, only emit the annotation if it didn't exist already.
  // Otherwise we risk confusing consumers

#define EVALUATE_ANNOTATION(step_category, step_type, precondition,            \
                            destination, annotation_value)                     \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  assert(step_category.relative_instance_location.empty());                    \
  const auto &target{context.resolve_target()};                                \
  if (!(precondition)) {                                                       \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  const auto annotation_result{                                                \
      context.annotate(destination, annotation_value)};                        \
  context.push(step_category);                                                 \
  if (annotation_result.second && step_category.report &&                      \
      callback.has_value()) {                                                  \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), destination, context.null);      \
    callback.value()(SchemaCompilerEvaluationType::Post, true, step,           \
                     context.evaluate_path(), destination,                     \
                     annotation_result.first);                                 \
  }                                                                            \
  context.pop(step_category);                                                  \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return true;

#define EVALUATE_ANNOTATION_NO_PRECONDITION(step_category, step_type,          \
                                            destination, annotation_value)     \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  const auto annotation_result{                                                \
      context.annotate(destination, annotation_value)};                        \
  context.push(step_category);                                                 \
  if (annotation_result.second && step_category.report &&                      \
      callback.has_value()) {                                                  \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), destination, context.null);      \
    callback.value()(SchemaCompilerEvaluationType::Post, true, step,           \
                     context.evaluate_path(), destination,                     \
                     annotation_result.first);                                 \
  }                                                                            \
  context.pop(step_category);                                                  \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return true;

  if (IS_STEP(SchemaCompilerAssertionFail)) {
    EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionFail);
    EVALUATE_END(assertion, SchemaCompilerAssertionFail);
  } else if (IS_STEP(SchemaCompilerAssertionDefines)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionDefines,
                   target.is_object());
    result = target.defines(assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionDefines);
  } else if (IS_STEP(SchemaCompilerAssertionDefinesAll)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionDefinesAll,
                   target.is_object());

    // Otherwise we are we even emitting this instruction?
    assert(assertion.value.size() > 1);
    result = true;
    for (const auto &property : assertion.value) {
      if (!target.defines(property)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(assertion, SchemaCompilerAssertionDefinesAll);
  } else if (IS_STEP(SchemaCompilerAssertionPropertyDependencies)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionPropertyDependencies,
                   target.is_object());
    // Otherwise we are we even emitting this instruction?
    assert(!assertion.value.empty());
    result = true;
    for (const auto &[property, dependencies] : assertion.value) {
      if (!target.defines(property)) {
        continue;
      }

      assert(!dependencies.empty());
      for (const auto &dependency : dependencies) {
        if (!target.defines(dependency)) {
          result = false;
          // For efficiently breaking from the outer loop too
          goto evaluate_assertion_property_dependencies_end;
        }
      }
    }

  evaluate_assertion_property_dependencies_end:
    EVALUATE_END(assertion, SchemaCompilerAssertionPropertyDependencies);
  } else if (IS_STEP(SchemaCompilerAssertionType)) {
    EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionType);
    const auto &target{context.resolve_target()};
    // In non-strict mode, we consider a real number that represents an
    // integer to be an integer
    result =
        target.type() == assertion.value ||
        (assertion.value == JSON::Type::Integer && target.is_integer_real());
    EVALUATE_END(assertion, SchemaCompilerAssertionType);
  } else if (IS_STEP(SchemaCompilerAssertionTypeAny)) {
    EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionTypeAny);
    // Otherwise we are we even emitting this instruction?
    assert(assertion.value.size() > 1);
    const auto &target{context.resolve_target()};
    // In non-strict mode, we consider a real number that represents an
    // integer to be an integer
    for (const auto type : assertion.value) {
      if (type == JSON::Type::Integer && target.is_integer_real()) {
        result = true;
        break;
      } else if (type == target.type()) {
        result = true;
        break;
      }
    }

    EVALUATE_END(assertion, SchemaCompilerAssertionTypeAny);
  } else if (IS_STEP(SchemaCompilerAssertionTypeStrict)) {
    EVALUATE_BEGIN_NO_PRECONDITION(assertion,
                                   SchemaCompilerAssertionTypeStrict);
    result = context.resolve_target().type() == assertion.value;
    EVALUATE_END(assertion, SchemaCompilerAssertionTypeStrict);
  } else if (IS_STEP(SchemaCompilerAssertionTypeStrictAny)) {
    EVALUATE_BEGIN_NO_PRECONDITION(assertion,
                                   SchemaCompilerAssertionTypeStrictAny);
    // Otherwise we are we even emitting this instruction?
    assert(assertion.value.size() > 1);
    result =
        (std::find(assertion.value.cbegin(), assertion.value.cend(),
                   context.resolve_target().type()) != assertion.value.cend());
    EVALUATE_END(assertion, SchemaCompilerAssertionTypeStrictAny);
  } else if (IS_STEP(SchemaCompilerAssertionRegex)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionRegex, target.is_string());
    result = std::regex_search(target.to_string(), assertion.value.first);
    EVALUATE_END(assertion, SchemaCompilerAssertionRegex);
  } else if (IS_STEP(SchemaCompilerAssertionStringSizeLess)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionStringSizeLess,
                   target.is_string());
    result = (target.size() < assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionStringSizeLess);
  } else if (IS_STEP(SchemaCompilerAssertionStringSizeGreater)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionStringSizeGreater,
                   target.is_string());
    result = (target.size() > assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionStringSizeGreater);
  } else if (IS_STEP(SchemaCompilerAssertionArraySizeLess)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionArraySizeLess,
                   target.is_array());
    result = (target.size() < assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionArraySizeLess);
  } else if (IS_STEP(SchemaCompilerAssertionArraySizeGreater)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionArraySizeGreater,
                   target.is_array());
    result = (target.size() > assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionArraySizeGreater);
  } else if (IS_STEP(SchemaCompilerAssertionObjectSizeLess)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionObjectSizeLess,
                   target.is_object());
    result = (target.size() < assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionObjectSizeLess);
  } else if (IS_STEP(SchemaCompilerAssertionObjectSizeGreater)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionObjectSizeGreater,
                   target.is_object());
    result = (target.size() > assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionObjectSizeGreater);
  } else if (IS_STEP(SchemaCompilerAssertionEqual)) {
    EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionEqual);
    result = (context.resolve_target() == assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionEqual);
  } else if (IS_STEP(SchemaCompilerAssertionEqualsAny)) {
    EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionEqualsAny);
    result = (std::find(assertion.value.cbegin(), assertion.value.cend(),
                        context.resolve_target()) != assertion.value.cend());
    EVALUATE_END(assertion, SchemaCompilerAssertionEqualsAny);
  } else if (IS_STEP(SchemaCompilerAssertionGreaterEqual)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionGreaterEqual,
                   target.is_number());
    result = target >= assertion.value;
    EVALUATE_END(assertion, SchemaCompilerAssertionGreaterEqual);
  } else if (IS_STEP(SchemaCompilerAssertionLessEqual)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionLessEqual,
                   target.is_number());
    result = target <= assertion.value;
    EVALUATE_END(assertion, SchemaCompilerAssertionLessEqual);
  } else if (IS_STEP(SchemaCompilerAssertionGreater)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionGreater,
                   target.is_number());
    result = target > assertion.value;
    EVALUATE_END(assertion, SchemaCompilerAssertionGreater);
  } else if (IS_STEP(SchemaCompilerAssertionLess)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionLess, target.is_number());
    result = target < assertion.value;
    EVALUATE_END(assertion, SchemaCompilerAssertionLess);
  } else if (IS_STEP(SchemaCompilerAssertionUnique)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionUnique, target.is_array());
    result = target.unique();
    EVALUATE_END(assertion, SchemaCompilerAssertionUnique);
  } else if (IS_STEP(SchemaCompilerAssertionDivisible)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionDivisible,
                   target.is_number());
    assert(assertion.value.is_number());
    result = target.divisible_by(assertion.value);
    EVALUATE_END(assertion, SchemaCompilerAssertionDivisible);
  } else if (IS_STEP(SchemaCompilerAssertionStringType)) {
    EVALUATE_BEGIN(assertion, SchemaCompilerAssertionStringType,
                   target.is_string());
    switch (assertion.value) {
      case SchemaCompilerValueStringType::URI:
        try {
          // TODO: This implies a string copy
          result = URI{target.to_string()}.is_absolute();
        } catch (const URIParseError &) {
          result = false;
        }

        break;
      default:
        // We should never get here
        assert(false);
    }

    EVALUATE_END(assertion, SchemaCompilerAssertionStringType);
  } else if (IS_STEP(SchemaCompilerAssertionPropertyType)) {
    EVALUATE_BEGIN_NO_TARGET(assertion, SchemaCompilerAssertionPropertyType,
                             // Note that here are are referring to the parent
                             // object that might hold the given property,
                             // before traversing into the actual property
                             context.resolve_target().is_object() &&
                                 has(context.resolve_target(),
                                     assertion.relative_instance_location));
    // Now here we refer to the actual property
    const auto &target{context.resolve_target()};
    // In non-strict mode, we consider a real number that represents an
    // integer to be an integer
    result =
        target.type() == assertion.value ||
        (assertion.value == JSON::Type::Integer && target.is_integer_real());
    EVALUATE_END(assertion, SchemaCompilerAssertionPropertyType);
  } else if (IS_STEP(SchemaCompilerAssertionPropertyTypeStrict)) {
    EVALUATE_BEGIN_NO_TARGET(assertion,
                             SchemaCompilerAssertionPropertyTypeStrict,
                             // Note that here are are referring to the parent
                             // object that might hold the given property,
                             // before traversing into the actual property
                             context.resolve_target().is_object() &&
                                 has(context.resolve_target(),
                                     assertion.relative_instance_location));
    // Now here we refer to the actual property
    result = context.resolve_target().type() == assertion.value;
    EVALUATE_END(assertion, SchemaCompilerAssertionPropertyTypeStrict);
  } else if (IS_STEP(SchemaCompilerLogicalOr)) {
    EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalOr);
    result = logical.children.empty();
    for (const auto &child : logical.children) {
      if (evaluate_step(child, mode, callback, context)) {
        result = true;
        // This boolean value controls whether we should still evaluate
        // every disjunction even on fast mode
        if (mode == SchemaCompilerEvaluationMode::Fast && !logical.value) {
          break;
        }
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalOr);
  } else if (IS_STEP(SchemaCompilerLogicalAnd)) {
    EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalAnd);
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalAnd);
  } else if (IS_STEP(SchemaCompilerLogicalWhenType)) {
    EVALUATE_BEGIN(logical, SchemaCompilerLogicalWhenType,
                   target.type() == logical.value);
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalWhenType);
  } else if (IS_STEP(SchemaCompilerLogicalWhenDefines)) {
    EVALUATE_BEGIN(logical, SchemaCompilerLogicalWhenDefines,
                   target.is_object() && target.defines(logical.value));
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalWhenDefines);
  } else if (IS_STEP(SchemaCompilerLogicalWhenAdjacentUnmarked)) {
    EVALUATE_BEGIN_NO_TARGET(logical, SchemaCompilerLogicalWhenAdjacentUnmarked,
                             !context.defines_any_adjacent_annotation(
                                 context.instance_location(),
                                 context.evaluate_path(), logical.value));
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalWhenAdjacentUnmarked);
  } else if (IS_STEP(SchemaCompilerLogicalWhenAdjacentMarked)) {
    EVALUATE_BEGIN_NO_TARGET(logical, SchemaCompilerLogicalWhenAdjacentMarked,
                             context.defines_any_adjacent_annotation(
                                 context.instance_location(),
                                 context.evaluate_path(), logical.value));
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalWhenAdjacentMarked);
  } else if (IS_STEP(SchemaCompilerLogicalWhenArraySizeGreater)) {
    EVALUATE_BEGIN(logical, SchemaCompilerLogicalWhenArraySizeGreater,
                   target.is_array() && target.size() > logical.value);
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalWhenArraySizeGreater);
  } else if (IS_STEP(SchemaCompilerLogicalWhenArraySizeEqual)) {
    EVALUATE_BEGIN(logical, SchemaCompilerLogicalWhenArraySizeEqual,
                   target.is_array() && target.size() == logical.value);
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalWhenArraySizeEqual);
  } else if (IS_STEP(SchemaCompilerLogicalXor)) {
    EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalXor);
    result = false;

    // TODO: Cache results of a given branch so we can avoid
    // computing it multiple times
    for (auto iterator{logical.children.cbegin()};
         iterator != logical.children.cend(); ++iterator) {
      if (!evaluate_step(*iterator, mode, callback, context)) {
        continue;
      }

      // Check if another one matches
      bool subresult{true};
      for (auto subiterator{logical.children.cbegin()};
           subiterator != logical.children.cend(); ++subiterator) {
        // Don't compare the element against itself
        if (std::distance(logical.children.cbegin(), iterator) ==
            std::distance(logical.children.cbegin(), subiterator)) {
          continue;
        }

        // We don't need to report traces that part of the exhaustive
        // XOR search. We can treat those as internal
        if (evaluate_step(*subiterator, mode, std::nullopt, context)) {
          subresult = false;
          break;
        }
      }

      result = result || subresult;
      if (result && mode == SchemaCompilerEvaluationMode::Fast) {
        break;
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalXor);
  } else if (IS_STEP(SchemaCompilerLogicalTryMark)) {
    EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalTryMark);
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    if (result) {
      // TODO: This implies an allocation of a JSON boolean
      context.annotate(context.instance_location(), JSON{true});
    } else {
      result = true;
    }

    EVALUATE_END(logical, SchemaCompilerLogicalTryMark);
  } else if (IS_STEP(SchemaCompilerLogicalNot)) {
    EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalNot);
    // Ignore annotations produced inside "not"
    context.mask();
    result = false;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = true;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }

    EVALUATE_END(logical, SchemaCompilerLogicalNot);
  } else if (IS_STEP(SchemaCompilerControlLabel)) {
    EVALUATE_BEGIN_NO_PRECONDITION(control, SchemaCompilerControlLabel);
    context.mark(control.value, control.children);
    result = true;
    for (const auto &child : control.children) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(control, SchemaCompilerControlLabel);
  } else if (IS_STEP(SchemaCompilerControlMark)) {
    SOURCEMETA_TRACE_START(trace_id, "SchemaCompilerControlMark");
    const auto &control{std::get<SchemaCompilerControlMark>(step)};
    context.mark(control.value, control.children);
    SOURCEMETA_TRACE_END(trace_id, "SchemaCompilerControlMark");
    return true;
  } else if (IS_STEP(SchemaCompilerControlJump)) {
    EVALUATE_BEGIN_NO_PRECONDITION(control, SchemaCompilerControlJump);
    result = true;
    for (const auto &child : context.jump(control.value)) {
      if (!evaluate_step(child, mode, callback, context)) {
        result = false;
        break;
      }
    }

    EVALUATE_END(control, SchemaCompilerControlJump);
  } else if (IS_STEP(SchemaCompilerControlDynamicAnchorJump)) {
    EVALUATE_BEGIN_NO_PRECONDITION(control,
                                   SchemaCompilerControlDynamicAnchorJump);
    const auto id{context.find_dynamic_anchor(control.value)};
    result = id.has_value();
    if (id.has_value()) {
      for (const auto &child : context.jump(id.value())) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          break;
        }
      }
    }

    EVALUATE_END(control, SchemaCompilerControlDynamicAnchorJump);
  } else if (IS_STEP(SchemaCompilerAnnotationEmit)) {
    EVALUATE_ANNOTATION_NO_PRECONDITION(
        annotation, SchemaCompilerAnnotationEmit, context.instance_location(),
        annotation.value);
  } else if (IS_STEP(SchemaCompilerAnnotationWhenArraySizeEqual)) {
    EVALUATE_ANNOTATION(annotation, SchemaCompilerAnnotationWhenArraySizeEqual,
                        target.is_array() &&
                            target.size() == annotation.value.first,
                        context.instance_location(), annotation.value.second);
  } else if (IS_STEP(SchemaCompilerAnnotationWhenArraySizeGreater)) {
    EVALUATE_ANNOTATION(
        annotation, SchemaCompilerAnnotationWhenArraySizeGreater,
        target.is_array() && target.size() > annotation.value.first,
        context.instance_location(), annotation.value.second);
  } else if (IS_STEP(SchemaCompilerAnnotationToParent)) {
    EVALUATE_ANNOTATION_NO_PRECONDITION(
        annotation, SchemaCompilerAnnotationToParent,
        // TODO: Can we avoid a copy of the instance location here?
        context.instance_location().initial(), annotation.value);
  } else if (IS_STEP(SchemaCompilerAnnotationBasenameToParent)) {
    EVALUATE_ANNOTATION_NO_PRECONDITION(
        annotation, SchemaCompilerAnnotationBasenameToParent,
        // TODO: Can we avoid a copy of the instance location here?
        context.instance_location().initial(),
        context.instance_location().back().to_json());
  } else if (IS_STEP(SchemaCompilerLoopPropertiesMatch)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesMatch, target.is_object());
    assert(!loop.value.empty());
    result = true;
    for (const auto &entry : target.as_object()) {
      const auto index{loop.value.find(entry.first)};
      if (index == loop.value.cend()) {
        continue;
      }

      const auto &substep{loop.children[index->second]};
      assert(std::holds_alternative<SchemaCompilerLogicalAnd>(substep));
      for (const auto &child :
           std::get<SchemaCompilerLogicalAnd>(substep).children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          // For efficiently breaking from the outer loop too
          goto evaluate_loop_properties_match_end;
        }
      }
    }

  evaluate_loop_properties_match_end:
    EVALUATE_END(loop, SchemaCompilerLoopPropertiesMatch);
  } else if (IS_STEP(SchemaCompilerLoopProperties)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopProperties, target.is_object());
    result = true;
    for (const auto &entry : target.as_object()) {
      context.enter(entry.first);
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          context.leave();
          // For efficiently breaking from the outer loop too
          goto evaluate_loop_properties_end;
        }
      }

      context.leave();
    }

  evaluate_loop_properties_end:
    EVALUATE_END(loop, SchemaCompilerLoopProperties);
  } else if (IS_STEP(SchemaCompilerLoopPropertiesRegex)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesRegex, target.is_object());
    result = true;
    for (const auto &entry : target.as_object()) {
      if (!std::regex_search(entry.first, loop.value.first)) {
        continue;
      }

      context.enter(entry.first);
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          context.leave();
          // For efficiently breaking from the outer loop too
          goto evaluate_loop_properties_regex_end;
        }
      }

      context.leave();
    }

  evaluate_loop_properties_regex_end:
    EVALUATE_END(loop, SchemaCompilerLoopPropertiesRegex);
  } else if (IS_STEP(SchemaCompilerLoopPropertiesNoAnnotation)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesNoAnnotation,
                   target.is_object());
    result = true;
    assert(!loop.value.empty());

    for (const auto &entry : target.as_object()) {
      // TODO: It might be more efficient to get all the annotations we
      // potentially care about as a set first, and the make the loop
      // check for O(1) containment in that set?
      if (context.defines_annotation(
              context.instance_location(),
              // TODO: Can we avoid doing this expensive operation on a loop?
              context.evaluate_path().initial(), loop.value,
              // TODO: This conversion implies a string copy
              JSON{entry.first})) {
        continue;
      }

      context.enter(entry.first);
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          context.leave();
          // For efficiently breaking from the outer loop too
          goto evaluate_loop_properties_no_annotation_end;
        }
      }

      context.leave();
    }

  evaluate_loop_properties_no_annotation_end:
    EVALUATE_END(loop, SchemaCompilerLoopPropertiesNoAnnotation);
  } else if (IS_STEP(SchemaCompilerLoopPropertiesExcept)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesExcept,
                   target.is_object());
    result = true;
    // Otherwise why emit this instruction?
    assert(!loop.value.first.empty() || !loop.value.second.empty());

    for (const auto &entry : target.as_object()) {
      if (loop.value.first.contains(entry.first) ||
          std::any_of(loop.value.second.cbegin(), loop.value.second.cend(),
                      [&entry](const auto &pattern) {
                        return std::regex_search(entry.first, pattern.first);
                      })) {
        continue;
      }

      context.enter(entry.first);
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          context.leave();
          // For efficiently breaking from the outer loop too
          goto evaluate_loop_properties_except_end;
        }
      }

      context.leave();
    }

  evaluate_loop_properties_except_end:
    EVALUATE_END(loop, SchemaCompilerLoopPropertiesExcept);
  } else if (IS_STEP(SchemaCompilerLoopKeys)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopKeys, target.is_object());
    result = true;
    context.target_type(EvaluationContext::TargetType::Key);
    for (const auto &entry : target.as_object()) {
      context.enter(entry.first);
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          context.leave();
          goto evaluate_loop_keys_end;
        }
      }

      context.leave();
    }

  evaluate_loop_keys_end:
    context.target_type(EvaluationContext::TargetType::Value);
    EVALUATE_END(loop, SchemaCompilerLoopKeys);
  } else if (IS_STEP(SchemaCompilerLoopItems)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopItems, target.is_array());
    const auto &array{target.as_array()};
    result = true;
    auto iterator{array.cbegin()};

    // We need this check, as advancing an iterator past its bounds
    // is considered undefined behavior
    // See https://en.cppreference.com/w/cpp/iterator/advance
    std::advance(iterator,
                 std::min(static_cast<std::ptrdiff_t>(loop.value),
                          static_cast<std::ptrdiff_t>(target.size())));

    for (; iterator != array.cend(); ++iterator) {
      const auto index{std::distance(array.cbegin(), iterator)};
      context.enter(static_cast<Pointer::Token::Index>(index));
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          context.leave();
          goto evaluate_compiler_loop_items_end;
        }
      }

      context.leave();
    }

  evaluate_compiler_loop_items_end:
    EVALUATE_END(loop, SchemaCompilerLoopItems);
  } else if (IS_STEP(SchemaCompilerLoopItemsUnmarked)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopItemsUnmarked,
                   target.is_array() &&
                       !context.defines_annotation(context.instance_location(),
                                                   context.evaluate_path(),
                                                   loop.value, JSON{true}));
    // Otherwise you shouldn't be using this step?
    assert(!loop.value.empty());
    const auto &array{target.as_array()};
    result = true;

    for (auto iterator = array.cbegin(); iterator != array.cend(); ++iterator) {
      const auto index{std::distance(array.cbegin(), iterator)};
      context.enter(static_cast<Pointer::Token::Index>(index));
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          context.leave();
          goto evaluate_compiler_loop_items_unmarked_end;
        }
      }

      context.leave();
    }

  evaluate_compiler_loop_items_unmarked_end:
    EVALUATE_END(loop, SchemaCompilerLoopItemsUnmarked);
  } else if (IS_STEP(SchemaCompilerLoopItemsUnevaluated)) {
    // TODO: This precondition is very expensive due to pointer manipulation
    EVALUATE_BEGIN(loop, SchemaCompilerLoopItemsUnevaluated,
                   target.is_array() && !context.defines_annotation(
                                            context.instance_location(),
                                            context.evaluate_path().initial(),
                                            loop.value.mask, JSON{true}));
    const auto &array{target.as_array()};
    result = true;
    auto iterator{array.cbegin()};

    // Determine the proper start based on integer annotations collected for the
    // current instance location by the keyword requested by the user.
    const std::uint64_t start{context.largest_annotation_index(
        context.instance_location(), {loop.value.index}, 0)};

    // We need this check, as advancing an iterator past its bounds
    // is considered undefined behavior
    // See https://en.cppreference.com/w/cpp/iterator/advance
    std::advance(iterator,
                 std::min(static_cast<std::ptrdiff_t>(start),
                          static_cast<std::ptrdiff_t>(target.size())));

    for (; iterator != array.cend(); ++iterator) {
      const auto index{std::distance(array.cbegin(), iterator)};

      if (context.defines_annotation(
              context.instance_location(),
              // TODO: Can we avoid doing this expensive operation on a loop?
              context.evaluate_path().initial(), loop.value.filter,
              JSON{static_cast<std::size_t>(index)})) {
        continue;
      }

      context.enter(static_cast<Pointer::Token::Index>(index));
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          context.leave();
          goto evaluate_compiler_loop_items_unevaluated_end;
        }
      }

      context.leave();
    }

  evaluate_compiler_loop_items_unevaluated_end:
    EVALUATE_END(loop, SchemaCompilerLoopItemsUnevaluated);
  } else if (IS_STEP(SchemaCompilerLoopContains)) {
    EVALUATE_BEGIN(loop, SchemaCompilerLoopContains, target.is_array());
    const auto minimum{std::get<0>(loop.value)};
    const auto &maximum{std::get<1>(loop.value)};
    assert(!maximum.has_value() || maximum.value() >= minimum);
    const auto is_exhaustive{std::get<2>(loop.value)};
    result = minimum == 0 && target.empty();
    const auto &array{target.as_array()};
    auto match_count{std::numeric_limits<decltype(minimum)>::min()};
    for (auto iterator = array.cbegin(); iterator != array.cend(); ++iterator) {
      const auto index{std::distance(array.cbegin(), iterator)};
      context.enter(static_cast<Pointer::Token::Index>(index));
      bool subresult{true};
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          subresult = false;
          break;
        }
      }

      context.leave();

      if (subresult) {
        match_count += 1;

        // Exceeding the upper bound is definitely a failure
        if (maximum.has_value() && match_count > maximum.value()) {
          result = false;

          // Note that here we don't want to consider whether to run
          // exhaustively or not. At this point, its already a failure,
          // and anything that comes after would not run at all anyway
          break;
        }

        if (match_count >= minimum) {
          result = true;

          // Exceeding the lower bound when there is no upper bound
          // is definitely a success
          if (!maximum.has_value() && !is_exhaustive) {
            break;
          }
        }
      }
    }

    EVALUATE_END(loop, SchemaCompilerLoopContains);
  }

#undef STRINGIFY
#undef IS_STEP
#undef EVALUATE_BEGIN
#undef EVALUATE_BEGIN_NO_TARGET
#undef EVALUATE_BEGIN_NO_PRECONDITION
#undef EVALUATE_END
#undef EVALUATE_ANNOTATION
#undef EVALUATE_ANNOTATION_NO_PRECONDITION
  // We should never get here
  assert(false);
  return false;
}

inline auto evaluate_internal(
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate &steps,
    const sourcemeta::jsontoolkit::JSON &instance,
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode mode,
    const std::optional<
        sourcemeta::jsontoolkit::SchemaCompilerEvaluationCallback> &callback)
    -> bool {
  EvaluationContext context{instance};
  bool overall{true};
  for (const auto &step : steps) {
    if (!evaluate_step(step, mode, callback, context)) {
      overall = false;
      break;
    }
  }

  // The evaluation path and instance location must be empty by the time
  // we are done, otherwise there was a frame push/pop mismatch
  assert(context.evaluate_path().empty());
  assert(context.instance_location().empty());
  assert(context.resources().empty());
  // We should end up at the root of the instance
  assert(context.instances().size() == 1);
  return overall;
}

} // namespace

namespace sourcemeta::jsontoolkit {

auto evaluate(const SchemaCompilerTemplate &steps, const JSON &instance,
              const SchemaCompilerEvaluationMode mode,
              const SchemaCompilerEvaluationCallback &callback) -> bool {
  return evaluate_internal(steps, instance, mode, callback);
}

auto evaluate(const SchemaCompilerTemplate &steps,
              const JSON &instance) -> bool {
  return evaluate_internal(steps, instance,
                           // Otherwise what's the point of an exhaustive
                           // evaluation if you don't get the results?
                           SchemaCompilerEvaluationMode::Fast, std::nullopt);
}

} // namespace sourcemeta::jsontoolkit
