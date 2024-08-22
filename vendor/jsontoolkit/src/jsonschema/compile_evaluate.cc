#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <algorithm>   // std::min
#include <cassert>     // assert
#include <functional>  // std::reference_wrapper
#include <iterator>    // std::distance, std::advance
#include <limits>      // std::numeric_limits
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <stack>       // std::stack
#include <type_traits> // std::is_same_v
#include <vector>      // std::vector

namespace {

class EvaluationContext {
public:
  using Pointer = sourcemeta::jsontoolkit::Pointer;
  using JSON = sourcemeta::jsontoolkit::JSON;
  using Annotations = std::set<JSON>;
  using InstanceAnnotations = std::map<Pointer, Annotations>;
  using Template = sourcemeta::jsontoolkit::SchemaCompilerTemplate;
  enum class TargetType { Value, Key };

  template <typename T> auto value(T &&document) -> const JSON & {
    return *(this->values.emplace(std::forward<T>(document)).first);
  }

  auto annotate(const Pointer &current_instance_location, JSON &&value)
      -> std::pair<std::reference_wrapper<const JSON>, bool> {
    const auto result{this->annotations_.insert({current_instance_location, {}})
                          .first->second.insert({this->evaluate_path(), {}})
                          .first->second.insert(std::move(value))};
    return {*(result.first), result.second};
  }

  auto
  annotations(const Pointer &current_instance_location,
              const Pointer &schema_location) const -> const Annotations & {
    static const Annotations placeholder;
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

  auto annotations(const Pointer &current_instance_location) const
      -> const InstanceAnnotations & {
    static const InstanceAnnotations placeholder;
    // Use `.find()` instead of `.contains()` and `.at()` for performance
    // reasons
    const auto instance_location_result{
        this->annotations_.find(current_instance_location)};
    if (instance_location_result == this->annotations_.end()) {
      return placeholder;
    }

    return instance_location_result->second;
  }

  template <typename T>
  auto push(const T &step, const Pointer &relative_evaluate_path,
            const Pointer &relative_instance_location) -> void {
    this->frame_sizes.emplace(relative_evaluate_path.size(),
                              relative_instance_location.size());
    this->evaluate_path_.push_back(relative_evaluate_path);
    this->instance_location_.push_back(relative_instance_location);

    // TODO: Do schema resource management using hashes to avoid
    // expensive string comparisons
    if (step.dynamic) {
      // Note that we are potentially repeatedly pushing back the
      // same schema resource over and over again. However, the
      // logic for making sure this list is "pure" takes a lot of
      // computation power. Being silly seems faster.
      this->resources_.push_back(step.schema_resource);
    }
  }

  template <typename T> auto push(const T &step) -> void {
    this->push(step, step.relative_schema_location,
               step.relative_instance_location);
  }

  template <typename T> auto pop(const T &step) -> void {
    assert(!this->frame_sizes.empty());
    const auto &sizes{this->frame_sizes.top()};
    this->evaluate_path_.pop_back(sizes.first);
    this->instance_location_.pop_back(sizes.second);
    this->frame_sizes.pop();

    // TODO: Do schema resource management using hashes to avoid
    // expensive string comparisons
    if (step.dynamic) {
      assert(!this->resources_.empty());
      this->resources_.pop_back();
    }
  }

  auto resources() const -> const std::vector<std::string> & {
    return this->resources_;
  }

  auto evaluate_path() const -> const Pointer & { return this->evaluate_path_; }

  auto instance_location() const -> const Pointer & {
    return this->instance_location_;
  }

  auto instance_location(const sourcemeta::jsontoolkit::SchemaCompilerTarget
                             &target) const -> Pointer {
    switch (target.first) {
      case sourcemeta::jsontoolkit::SchemaCompilerTargetType::InstanceParent:
        return this->instance_location().concat(target.second).initial();
      default:
        return this->instance_location().concat(target.second);
    }
  }

  template <typename T> auto instance_location(const T &step) const -> Pointer {
    return this->instance_location(step.target);
  }

  auto target_type(const TargetType type) -> void { this->target_type_ = type; }

  auto target_type() const -> TargetType { return this->target_type_; }

  template <typename T>
  auto
  resolve_target(const sourcemeta::jsontoolkit::SchemaCompilerTarget &target,
                 const JSON &instance) -> const T & {
    using namespace sourcemeta::jsontoolkit;

    // An optimization for efficiently accessing annotations
    if constexpr (std::is_same_v<Annotations, T>) {
      const auto schema_location{
          this->evaluate_path().initial().concat(target.second)};
      assert(target.first != SchemaCompilerTargetType::ParentAnnotations);
      if (target.first == SchemaCompilerTargetType::ParentAdjacentAnnotations) {
        return this->annotations(this->instance_location().initial(),
                                 schema_location);
      } else {
        return this->annotations(this->instance_location(), schema_location);
      }
    } else if constexpr (std::is_same_v<InstanceAnnotations, T>) {
      if (target.first == SchemaCompilerTargetType::ParentAnnotations) {
        return this->annotations(this->instance_location().initial());
      } else {
        assert(target.first == SchemaCompilerTargetType::Annotations);
        return this->annotations(this->instance_location());
      }
    } else {
      static_assert(std::is_same_v<JSON, T>);
      switch (target.first) {
        case SchemaCompilerTargetType::Instance:
          if (this->target_type() == TargetType::Key) {
            assert(!this->instance_location(target).empty());
            assert(this->instance_location(target).back().is_property());
            return this->value(
                this->instance_location(target).back().to_property());
          }

          assert(this->target_type() == TargetType::Value);
          return get(instance, this->instance_location(target));
        case SchemaCompilerTargetType::InstanceBasename:
          return this->value(this->instance_location(target).back().to_json());
        default:
          // We should never get here
          assert(false);
          return this->value(nullptr);
      }
    }
  }

  template <typename T>
  auto resolve_value(
      const sourcemeta::jsontoolkit::SchemaCompilerStepValue<T> &value,
      const JSON &instance) -> T {
    using namespace sourcemeta::jsontoolkit;
    // We only define target resolution for JSON documents, at least for now
    if constexpr (std::is_same_v<SchemaCompilerValueJSON, T>) {
      if (std::holds_alternative<SchemaCompilerTarget>(value)) {
        const auto &target{std::get<SchemaCompilerTarget>(value)};
        return this->resolve_target<T>(target, instance);
      }
    }

    assert(std::holds_alternative<T>(value));
    return std::get<T>(value);
  }

  auto mark(const std::size_t id, const Template &children) -> void {
    this->labels.try_emplace(id, children);
  }

  // TODO: At least currently, we only need to mask if a schema
  // makes use of `unevaluatedProperties` or `unevaluatedItems`
  // Detect if a schema does need this so if not, we avoid
  // an unnecessary copy
  auto mask() -> void {
    this->annotation_blacklist.insert(this->evaluate_path_);
  }

  auto masked(const Pointer &path) const -> bool {
    for (const auto &masked : this->annotation_blacklist) {
      if (path.starts_with(masked) &&
          !this->evaluate_path_.starts_with(masked)) {
        return true;
      }
    }

    return false;
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

  auto jump(const std::size_t id) const -> const Template & {
    assert(this->labels.contains(id));
    return this->labels.at(id).get();
  }

private:
  Pointer evaluate_path_;
  Pointer instance_location_;
  std::stack<std::pair<std::size_t, std::size_t>> frame_sizes;
  // TODO: Keep hashes of schema resources URI instead for performance reasons
  std::vector<std::string> resources_;
  std::set<Pointer> annotation_blacklist;
  // For efficiency, as we likely reference the same JSON values
  // over and over again
  std::set<JSON> values;
  // We don't use a pair for holding the two pointers for runtime
  // efficiency when resolving keywords like `unevaluatedProperties`
  std::map<Pointer, InstanceAnnotations> annotations_;
  std::map<std::size_t, const std::reference_wrapper<const Template>> labels;
  TargetType target_type_ = TargetType::Value;
};

auto callback_noop(
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationType, bool,
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &,
    const sourcemeta::jsontoolkit::Pointer &,
    const sourcemeta::jsontoolkit::Pointer &,
    const sourcemeta::jsontoolkit::JSON &) noexcept -> void {}

auto evaluate_step(
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &step,
    const sourcemeta::jsontoolkit::JSON &instance,
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode mode,
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationCallback &callback,
    EvaluationContext &context) -> bool {
  using namespace sourcemeta::jsontoolkit;
  bool result{false};

#define CALLBACK_PRE(current_instance_location)                                \
  callback(SchemaCompilerEvaluationType::Pre, true, step,                      \
           context.evaluate_path(), current_instance_location,                 \
           context.value(nullptr));

#define CALLBACK_POST(current_step)                                            \
  callback(SchemaCompilerEvaluationType::Post, result, step,                   \
           context.evaluate_path(), context.instance_location(),               \
           context.value(nullptr));                                            \
  context.pop(current_step);                                                   \
  return result;

#define EVALUATE_CONDITION_GUARD(step, instance)                               \
  for (const auto &child : step.condition) {                                   \
    if (!evaluate_step(child, instance, SchemaCompilerEvaluationMode::Fast,    \
                       callback_noop, context)) {                              \
      context.pop(step);                                                       \
      return true;                                                             \
    }                                                                          \
  }

  if (std::holds_alternative<SchemaCompilerAssertionFail>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionFail>(step)};
    context.push(assertion);
    assert(std::holds_alternative<SchemaCompilerValueNone>(assertion.value));
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionDefines>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionDefines>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target.is_object() && target.defines(value);
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionDefinesAll>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionDefinesAll>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    // Otherwise we are we even emitting this instruction?
    assert(value.size() > 1);
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    assert(target.is_object());
    result = true;
    for (const auto &property : value) {
      if (!target.defines(property)) {
        result = false;
        break;
      }
    }

    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionType>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionType>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    // In non-strict mode, we consider a real number that represents an
    // integer to be an integer
    result = target.type() == value ||
             (value == JSON::Type::Integer && target.is_integer_real());

    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionTypeAny>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionTypeAny>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    // Otherwise we are we even emitting this instruction?
    assert(value.size() > 1);
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    // In non-strict mode, we consider a real number that represents an
    // integer to be an integer
    result = value.contains(target.type()) ||
             (value.contains(JSON::Type::Integer) && target.is_integer_real());

    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionTypeStrict>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionTypeStrict>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target.type() == value;
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionTypeStrictAny>(
                 step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionTypeStrictAny>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    // Otherwise we are we even emitting this instruction?
    assert(value.size() > 1);
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = value.contains(target.type());
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionRegex>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionRegex>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    assert(target.is_string());
    result = std::regex_search(target.to_string(), value.first);
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionSizeGreater>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionSizeGreater>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = (target.is_array() || target.is_object() || target.is_string()) &&
             (target.size() > value);
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionSizeLess>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionSizeLess>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = (target.is_array() || target.is_object() || target.is_string()) &&
             (target.size() < value);
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerInternalSizeEqual>(step)) {
    const auto &assertion{std::get<SchemaCompilerInternalSizeEqual>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = (target.is_array() || target.is_object() || target.is_string()) &&
             (target.size() == value);
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionEqual>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionEqual>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = (target == value);
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionEqualsAny>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionEqualsAny>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = value.contains(target);
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionGreaterEqual>(
                 step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionGreaterEqual>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target >= value;
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionLessEqual>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionLessEqual>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target <= value;
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionGreater>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionGreater>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target > value;
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionLess>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionLess>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target < value;
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionUnique>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionUnique>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(assertion.value));
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target.is_array() && target.unique();
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionDivisible>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionDivisible>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    assert(value.is_number());
    assert(target.is_number());
    result = target.divisible_by(value);
    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerAssertionStringType>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionStringType>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    CALLBACK_PRE(context.instance_location());
    const auto value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    assert(target.is_string());
    switch (value) {
      case SchemaCompilerValueStringType::URI:
        try {
          result = URI{target.to_string()}.is_absolute();
        } catch (const URIParseError &) {
          result = false;
        }

        break;
      default:
        // We should never get here
        assert(false);
    }

    CALLBACK_POST(assertion);
  } else if (std::holds_alternative<SchemaCompilerLogicalOr>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalOr>(step)};
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical, instance);
    CALLBACK_PRE(context.instance_location());
    // This boolean value controls whether we should still evaluate
    // every disjunction even on fast mode
    const auto value{context.resolve_value(logical.value, instance)};
    result = logical.children.empty();
    for (const auto &child : logical.children) {
      if (evaluate_step(child, instance, mode, callback, context)) {
        result = true;
        if (mode == SchemaCompilerEvaluationMode::Fast && !value) {
          break;
        }
      }
    }

    CALLBACK_POST(logical);
  } else if (std::holds_alternative<SchemaCompilerLogicalAnd>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalAnd>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(logical.value));
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical, instance);
    CALLBACK_PRE(context.instance_location());
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = false;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }

    CALLBACK_POST(logical);
  } else if (std::holds_alternative<SchemaCompilerLogicalXor>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalXor>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(logical.value));
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical, instance);
    CALLBACK_PRE(context.instance_location());
    result = false;

    // TODO: Cache results of a given branch so we can avoid
    // computing it multiple times
    for (auto iterator{logical.children.cbegin()};
         iterator != logical.children.cend(); ++iterator) {
      if (!evaluate_step(*iterator, instance, mode, callback, context)) {
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
        if (evaluate_step(*subiterator, instance, mode, callback_noop,
                          context)) {
          subresult = false;
          break;
        }
      }

      result = result || subresult;
      if (result && mode == SchemaCompilerEvaluationMode::Fast) {
        break;
      }
    }

    CALLBACK_POST(logical);
  } else if (std::holds_alternative<SchemaCompilerLogicalTry>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalTry>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(logical.value));
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical, instance);
    CALLBACK_PRE(context.instance_location());
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        break;
      }
    }

    CALLBACK_POST(logical);
  } else if (std::holds_alternative<SchemaCompilerLogicalNot>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalNot>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(logical.value));
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical, instance);
    CALLBACK_PRE(context.instance_location());
    // Ignore annotations produced inside "not"
    context.mask();
    result = false;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = true;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }

    CALLBACK_POST(logical);
  } else if (std::holds_alternative<SchemaCompilerInternalAnnotation>(step)) {
    const auto &assertion{std::get<SchemaCompilerInternalAnnotation>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<std::set<JSON>>(assertion.target, instance)};
    result = target.contains(value);

    // We treat this step as transparent to the consumer
    context.pop(assertion);
    return result;
  } else if (std::holds_alternative<SchemaCompilerInternalNoAdjacentAnnotation>(
                 step)) {
    const auto &assertion{
        std::get<SchemaCompilerInternalNoAdjacentAnnotation>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<std::set<JSON>>(assertion.target, instance)};
    result = !target.contains(value);

    // We treat this step as transparent to the consumer
    context.pop(assertion);
    return result;
  } else if (std::holds_alternative<SchemaCompilerInternalNoAnnotation>(step)) {
    const auto &assertion{std::get<SchemaCompilerInternalNoAnnotation>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<EvaluationContext::InstanceAnnotations>(
            assertion.target, instance)};
    result = true;

    if (!assertion.data.empty()) {
      for (const auto &[schema_location, annotations] : target) {
        assert(!schema_location.empty());
        const auto &keyword{schema_location.back()};
        if (keyword.is_property() &&
            assertion.data.contains(keyword.to_property()) &&
            annotations.contains(value) &&
            // Make sure its not a cousin annotation, which can
            // never be seen
            // TODO: Have a better function at Pointer to check
            // for these "initial starts with" cases in a way
            // that we don't have to copy pointers, which `.initial()`
            // does.
            schema_location.initial().starts_with(
                context.evaluate_path().initial()) &&
            // We want to ignore certain annotations, like the ones
            // inside "not"
            !context.masked(schema_location)) {
          result = false;
          break;
        }
      }
    }

    // We treat this step as transparent to the consumer
    context.pop(assertion);
    return result;
  } else if (std::holds_alternative<SchemaCompilerInternalContainer>(step)) {
    const auto &container{std::get<SchemaCompilerInternalContainer>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(container.value));
    context.push(container);
    EVALUATE_CONDITION_GUARD(container, instance);
    result = true;
    for (const auto &child : container.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = false;
        break;
      }
    }

    // We treat this step as transparent to the consumer
    context.pop(container);
    return result;
  } else if (std::holds_alternative<SchemaCompilerInternalDefinesAll>(step)) {
    const auto &assertion{std::get<SchemaCompilerInternalDefinesAll>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    assert(target.is_object());
    result = true;
    for (const auto &property : value) {
      if (!target.defines(property)) {
        result = false;
        break;
      }
    }

    // We treat this step as transparent to the consumer
    context.pop(assertion);
    return result;
  } else if (std::holds_alternative<SchemaCompilerControlLabel>(step)) {
    const auto &control{std::get<SchemaCompilerControlLabel>(step)};
    context.mark(control.id, control.children);
    context.push(control);
    CALLBACK_PRE(context.instance_location());
    result = true;
    for (const auto &child : control.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = false;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }

    CALLBACK_POST(control);
  } else if (std::holds_alternative<SchemaCompilerControlMark>(step)) {
    const auto &control{std::get<SchemaCompilerControlMark>(step)};
    context.mark(control.id, control.children);
    return true;
  } else if (std::holds_alternative<SchemaCompilerControlJump>(step)) {
    const auto &control{std::get<SchemaCompilerControlJump>(step)};
    context.push(control);
    CALLBACK_PRE(context.instance_location());
    assert(control.children.empty());
    result = true;
    for (const auto &child : context.jump(control.id)) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = false;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }

    CALLBACK_POST(control);
  } else if (std::holds_alternative<SchemaCompilerControlDynamicAnchorJump>(
                 step)) {
    const auto &control{std::get<SchemaCompilerControlDynamicAnchorJump>(step)};
    context.push(control);
    CALLBACK_PRE(context.instance_location());
    const auto id{context.find_dynamic_anchor(control.id)};
    result = id.has_value();
    if (id.has_value()) {
      for (const auto &child : context.jump(id.value())) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          result = false;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            break;
          }
        }
      }
    }

    CALLBACK_POST(control);
  } else if (std::holds_alternative<SchemaCompilerAnnotationPublic>(step)) {
    const auto &annotation{std::get<SchemaCompilerAnnotationPublic>(step)};
    context.push(annotation);
    EVALUATE_CONDITION_GUARD(annotation, instance);
    const auto current_instance_location{context.instance_location(annotation)};
    const auto value{
        context.annotate(current_instance_location,
                         context.resolve_value(annotation.value, instance))};
    // Annotations never fail
    result = true;

    // As a safety guard, only emit the annotation if it didn't exist already.
    // Otherwise we risk confusing consumers
    if (value.second) {
      CALLBACK_PRE(current_instance_location);
      callback(SchemaCompilerEvaluationType::Post, result, step,
               context.evaluate_path(), current_instance_location, value.first);
    }

    context.pop(annotation);
    return result;
  } else if (std::holds_alternative<SchemaCompilerLoopProperties>(step)) {
    const auto &loop{std::get<SchemaCompilerLoopProperties>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop, instance);
    const auto value{context.resolve_value(loop.value, instance)};

    // Setting the value to false means "don't report it"
    if (value) {
      CALLBACK_PRE(context.instance_location());
    }

    const auto &target{context.resolve_target<JSON>(loop.target, instance)};
    assert(target.is_object());
    result = true;
    for (const auto &entry : target.as_object()) {
      context.push(loop, empty_pointer, {entry.first});
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          result = false;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            context.pop(loop);
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_end;
          } else {
            break;
          }
        }
      }

      context.pop(loop);
    }

  evaluate_loop_properties_end:
    // Setting the value to false means "don't report it"
    if (!value) {
      context.pop(loop);
      return result;
    }

    CALLBACK_POST(loop);
  } else if (std::holds_alternative<SchemaCompilerLoopKeys>(step)) {
    const auto &loop{std::get<SchemaCompilerLoopKeys>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop, instance);
    CALLBACK_PRE(context.instance_location());
    assert(std::holds_alternative<SchemaCompilerValueNone>(loop.value));
    const auto &target{context.resolve_target<JSON>(loop.target, instance)};
    assert(target.is_object());
    result = true;
    context.target_type(EvaluationContext::TargetType::Key);
    for (const auto &entry : target.as_object()) {
      context.push(loop, empty_pointer, {entry.first});
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          result = false;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            context.pop(loop);
            goto evaluate_loop_keys_end;
          } else {
            break;
          }
        }
      }

      context.pop(loop);
    }

  evaluate_loop_keys_end:
    context.target_type(EvaluationContext::TargetType::Value);
    CALLBACK_POST(loop);
  } else if (std::holds_alternative<SchemaCompilerLoopItems>(step)) {
    const auto &loop{std::get<SchemaCompilerLoopItems>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop, instance);
    CALLBACK_PRE(context.instance_location());
    const auto value{context.resolve_value(loop.value, instance)};
    const auto &target{context.resolve_target<JSON>(loop.target, instance)};
    assert(target.is_array());
    const auto &array{target.as_array()};
    result = true;
    auto iterator{array.cbegin()};

    // We need this check, as advancing an iterator past its bounds
    // is considered undefined behavior
    // See https://en.cppreference.com/w/cpp/iterator/advance
    std::advance(iterator,
                 std::min(static_cast<std::ptrdiff_t>(value),
                          static_cast<std::ptrdiff_t>(target.size())));

    for (; iterator != array.cend(); ++iterator) {
      const auto index{std::distance(array.cbegin(), iterator)};
      context.push(loop, empty_pointer,
                   {static_cast<Pointer::Token::Index>(index)});
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          result = false;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            context.pop(loop);
            CALLBACK_POST(loop);
          } else {
            break;
          }
        }
      }

      context.pop(loop);
    }

    CALLBACK_POST(loop);
  } else if (std::holds_alternative<SchemaCompilerLoopItemsFromAnnotationIndex>(
                 step)) {
    const auto &loop{
        std::get<SchemaCompilerLoopItemsFromAnnotationIndex>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(loop.value, instance)};
    const auto &target{context.resolve_target<JSON>(loop.target, instance)};
    assert(target.is_array());
    const auto &array{target.as_array()};
    result = true;
    auto iterator{array.cbegin()};

    // Determine the proper start based on integer annotations collected for the
    // current instance location by the keyword requested by the user. We will
    // exhaustively check the matching annotations and end up with the largest
    // index or zero
    std::uint64_t start{0};
    for (const auto &[schema_location, annotations] :
         context.annotations(context.instance_location())) {
      assert(!schema_location.empty());
      const auto &keyword{schema_location.back()};
      if (!keyword.is_property() || keyword.to_property() != value) {
        continue;
      }

      for (const auto &annotation : annotations) {
        if (annotation.is_integer() && annotation.is_positive()) {
          start = std::max(
              start, static_cast<std::uint64_t>(annotation.to_integer()) + 1);
        }
      }
    }

    // We need this check, as advancing an iterator past its bounds
    // is considered undefined behavior
    // See https://en.cppreference.com/w/cpp/iterator/advance
    std::advance(iterator,
                 std::min(static_cast<std::ptrdiff_t>(start),
                          static_cast<std::ptrdiff_t>(target.size())));

    for (; iterator != array.cend(); ++iterator) {
      const auto index{std::distance(array.cbegin(), iterator)};
      context.push(loop, empty_pointer,
                   {static_cast<Pointer::Token::Index>(index)});
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          result = false;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            context.pop(loop);
            CALLBACK_POST(loop);
          } else {
            break;
          }
        }
      }

      context.pop(loop);
    }

    CALLBACK_POST(loop);
  } else if (std::holds_alternative<SchemaCompilerLoopContains>(step)) {
    const auto &loop{std::get<SchemaCompilerLoopContains>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop, instance);
    CALLBACK_PRE(context.instance_location());
    const auto &value{context.resolve_value(loop.value, instance)};
    const auto minimum{std::get<0>(value)};
    const auto &maximum{std::get<1>(value)};
    assert(!maximum.has_value() || maximum.value() >= minimum);
    const auto is_exhaustive{std::get<2>(value)};
    const auto &target{context.resolve_target<JSON>(loop.target, instance)};
    assert(target.is_array());
    result = minimum == 0 && target.empty();
    const auto &array{target.as_array()};
    auto match_count{std::numeric_limits<decltype(minimum)>::min()};
    for (auto iterator = array.cbegin(); iterator != array.cend(); ++iterator) {
      const auto index{std::distance(array.cbegin(), iterator)};
      context.push(loop, empty_pointer,
                   {static_cast<Pointer::Token::Index>(index)});
      bool subresult{true};
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          subresult = false;
          break;
        }
      }

      context.pop(loop);

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

    CALLBACK_POST(loop);
  }

#undef CALLBACK_PRE
#undef CALLBACK_POST
#undef EVALUATE_CONDITION_GUARD
  // We should never get here
  assert(false);
  return result;
}

} // namespace

namespace sourcemeta::jsontoolkit {

auto evaluate(const SchemaCompilerTemplate &steps, const JSON &instance,
              const SchemaCompilerEvaluationMode mode,
              const SchemaCompilerEvaluationCallback &callback) -> bool {
  EvaluationContext context;
  bool overall{true};
  for (const auto &step : steps) {
    if (!evaluate_step(step, instance, mode, callback, context)) {
      overall = false;
      if (mode == SchemaCompilerEvaluationMode::Fast) {
        break;
      }
    }
  }

  // The evaluation path and instance location must be empty by the time
  // we are done, otherwise there was a frame push/pop mismatch
  assert(context.evaluate_path().empty());
  assert(context.instance_location().empty());
  assert(context.resources().empty());
  return overall;
}

auto evaluate(const SchemaCompilerTemplate &steps,
              const JSON &instance) -> bool {
  // Otherwise what's the point of an exhaustive
  // evaluation if you don't get the results?
  return evaluate(steps, instance, SchemaCompilerEvaluationMode::Fast,
                  callback_noop);
}

} // namespace sourcemeta::jsontoolkit
