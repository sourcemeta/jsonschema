#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <algorithm>   // std::min
#include <cassert>     // assert
#include <functional>  // std::reference_wrapper
#include <iterator>    // std::distance, std::advance
#include <map>         // std::map
#include <set>         // std::set
#include <type_traits> // std::is_same_v
#include <vector>      // std::vector

namespace {

class EvaluationContext {
public:
  using Pointer = sourcemeta::jsontoolkit::Pointer;
  using JSON = sourcemeta::jsontoolkit::JSON;
  using Annotations = std::set<JSON>;
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

  auto push(const Pointer &relative_evaluate_path,
            const Pointer &relative_instance_location) -> void {
    this->frame_sizes.emplace_back(relative_evaluate_path.size(),
                                   relative_instance_location.size());
    this->evaluate_path_.push_back(relative_evaluate_path);
    this->instance_location_.push_back(relative_instance_location);
  }

  template <typename T> auto push(const T &step) -> void {
    this->push(step.relative_schema_location, step.relative_instance_location);
  }

  auto pop() -> void {
    assert(!this->frame_sizes.empty());
    const auto &sizes{this->frame_sizes.back()};
    this->evaluate_path_.pop_back(sizes.first);
    this->instance_location_.pop_back(sizes.second);
    this->frame_sizes.pop_back();
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
      assert(target.first ==
             SchemaCompilerTargetType::ParentAdjacentAnnotations);
      const auto schema_location{
          this->evaluate_path().initial().concat(target.second)};
      return this->annotations(this->instance_location().initial(),
                               schema_location);
    } else {
      static_assert(std::is_same_v<JSON, T>);
      switch (target.first) {
        case SchemaCompilerTargetType::Instance:
          if (this->target_type() == TargetType::Key) {
            assert(!this->instance_location(target).empty());
            assert(this->instance_location(target).back().is_property());
            return this->value(
                JSON{this->instance_location(target).back().to_property()});
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
        return this->resolve_target<JSON>(target, instance);
      }
    }

    assert(std::holds_alternative<T>(value));
    return std::get<T>(value);
  }

  auto mark(const std::size_t id, const Template &children) -> void {
    this->labels.try_emplace(id, children);
  }

  auto jump(const std::size_t id) const -> const Template & {
    assert(this->labels.contains(id));
    return this->labels.at(id).get();
  }

private:
  Pointer evaluate_path_;
  Pointer instance_location_;
  std::vector<std::pair<std::size_t, std::size_t>> frame_sizes;
  // For efficiency, as we likely reference the same JSON values
  // over and over again
  std::set<JSON> values;
  // We don't use a pair for holding the two pointers for runtime
  // efficiency when resolving keywords like `unevaluatedProperties`
  std::map<Pointer, std::map<Pointer, Annotations>> annotations_;
  std::map<std::size_t, const std::reference_wrapper<const Template>> labels;
  TargetType target_type_ = TargetType::Value;
};

auto callback_noop(
    bool, const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &,
    const sourcemeta::jsontoolkit::Pointer &,
    const sourcemeta::jsontoolkit::Pointer &,
    const sourcemeta::jsontoolkit::JSON &,
    const sourcemeta::jsontoolkit::JSON &) noexcept -> void {}

auto evaluate_step(
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &step,
    const sourcemeta::jsontoolkit::JSON &instance,
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode mode,
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationCallback &callback,
    EvaluationContext &context) -> bool {
  using namespace sourcemeta::jsontoolkit;
  bool result{false};

#define EVALUATE_CONDITION_GUARD(condition, instance)                          \
  for (const auto &child : condition) {                                        \
    if (!evaluate_step(child, instance, SchemaCompilerEvaluationMode::Fast,    \
                       callback_noop, context)) {                              \
      context.pop();                                                           \
      return true;                                                             \
    }                                                                          \
  }

  if (std::holds_alternative<SchemaCompilerAssertionFail>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionFail>(step)};
    context.push(assertion);
    assert(std::holds_alternative<SchemaCompilerValueNone>(assertion.value));
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
  } else if (std::holds_alternative<SchemaCompilerAssertionDefines>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionDefines>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target.is_object() && target.defines(value);
  } else if (std::holds_alternative<SchemaCompilerAssertionDefinesAll>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionDefinesAll>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
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
  } else if (std::holds_alternative<SchemaCompilerAssertionType>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionType>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    // In non-strict mode, we consider a real number that represents an
    // integer to be an integer
    result = target.type() == value ||
             (value == JSON::Type::Integer && target.is_integer_real());
  } else if (std::holds_alternative<SchemaCompilerAssertionTypeAny>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionTypeAny>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    // In non-strict mode, we consider a real number that represents an
    // integer to be an integer
    result = value.contains(target.type()) ||
             (value.contains(JSON::Type::Integer) && target.is_integer_real());
  } else if (std::holds_alternative<SchemaCompilerAssertionTypeStrict>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionTypeStrict>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target.type() == value;
  } else if (std::holds_alternative<SchemaCompilerAssertionTypeStrictAny>(
                 step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionTypeStrictAny>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = value.contains(target.type());
  } else if (std::holds_alternative<SchemaCompilerAssertionRegex>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionRegex>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    assert(target.is_string());
    result = std::regex_search(target.to_string(), value.first);
  } else if (std::holds_alternative<SchemaCompilerAssertionSizeGreater>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionSizeGreater>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = (target.is_array() || target.is_object() || target.is_string()) &&
             (target.size() > value);
  } else if (std::holds_alternative<SchemaCompilerAssertionSizeLess>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionSizeLess>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = (target.is_array() || target.is_object() || target.is_string()) &&
             (target.size() < value);
  } else if (std::holds_alternative<SchemaCompilerAssertionEqual>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionEqual>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = (target == value);
  } else if (std::holds_alternative<SchemaCompilerAssertionEqualsAny>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionEqualsAny>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = value.contains(target);
  } else if (std::holds_alternative<SchemaCompilerAssertionGreaterEqual>(
                 step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionGreaterEqual>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target >= value;
  } else if (std::holds_alternative<SchemaCompilerAssertionLessEqual>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionLessEqual>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target <= value;
  } else if (std::holds_alternative<SchemaCompilerAssertionGreater>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionGreater>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target > value;
  } else if (std::holds_alternative<SchemaCompilerAssertionLess>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionLess>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target < value;
  } else if (std::holds_alternative<SchemaCompilerAssertionUnique>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionUnique>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(assertion.value));
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    result = target.is_array() && target.unique();
  } else if (std::holds_alternative<SchemaCompilerAssertionDivisible>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionDivisible>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<JSON>(assertion.target, instance)};
    assert(value.is_number());
    assert(target.is_number());
    result = target.divisible_by(value);
  } else if (std::holds_alternative<SchemaCompilerAssertionStringType>(step)) {
    const auto &assertion{std::get<SchemaCompilerAssertionStringType>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
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
  } else if (std::holds_alternative<SchemaCompilerLogicalOr>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalOr>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(logical.value));
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical.condition, instance);
    result = logical.children.empty();
    for (const auto &child : logical.children) {
      if (evaluate_step(child, instance, mode, callback, context)) {
        result = true;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }
  } else if (std::holds_alternative<SchemaCompilerLogicalAnd>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalAnd>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(logical.value));
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical.condition, instance);
    result = true;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = false;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }
  } else if (std::holds_alternative<SchemaCompilerLogicalXor>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalXor>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(logical.value));
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical.condition, instance);
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
  } else if (std::holds_alternative<SchemaCompilerLogicalNot>(step)) {
    const auto &logical{std::get<SchemaCompilerLogicalNot>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(logical.value));
    context.push(logical);
    EVALUATE_CONDITION_GUARD(logical.condition, instance);
    result = false;
    for (const auto &child : logical.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = true;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }
  } else if (std::holds_alternative<SchemaCompilerInternalNoAnnotation>(step)) {
    const auto &assertion{std::get<SchemaCompilerInternalNoAnnotation>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
    const auto &value{context.resolve_value(assertion.value, instance)};
    const auto &target{
        context.resolve_target<std::set<JSON>>(assertion.target, instance)};
    result = !target.contains(value);

    // We treat this step as transparent to the consumer
    context.pop();
    return result;
  } else if (std::holds_alternative<SchemaCompilerInternalContainer>(step)) {
    const auto &container{std::get<SchemaCompilerInternalContainer>(step)};
    assert(std::holds_alternative<SchemaCompilerValueNone>(container.value));
    context.push(container);
    EVALUATE_CONDITION_GUARD(container.condition, instance);
    result = true;
    for (const auto &child : container.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = false;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }

    // We treat this step as transparent to the consumer
    context.pop();
    return result;
  } else if (std::holds_alternative<SchemaCompilerInternalDefinesAll>(step)) {
    const auto &assertion{std::get<SchemaCompilerInternalDefinesAll>(step)};
    context.push(assertion);
    EVALUATE_CONDITION_GUARD(assertion.condition, instance);
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
    context.pop();
    return result;
  } else if (std::holds_alternative<SchemaCompilerControlLabel>(step)) {
    const auto &control{std::get<SchemaCompilerControlLabel>(step)};
    context.mark(control.id, control.children);
    context.push(control);
    result = true;
    for (const auto &child : control.children) {
      if (!evaluate_step(child, instance, mode, callback, context)) {
        result = false;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }
  } else if (std::holds_alternative<SchemaCompilerControlJump>(step)) {
    const auto &control{std::get<SchemaCompilerControlJump>(step)};
    context.push(control);
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
  } else if (std::holds_alternative<SchemaCompilerAnnotationPublic>(step)) {
    // Annotations never fail
    result = true;

    // No reasons to emit public annotations on this mode
    if (mode == SchemaCompilerEvaluationMode::Fast) {
      return result;
    }

    const auto &annotation{std::get<SchemaCompilerAnnotationPublic>(step)};
    context.push(annotation);
    EVALUATE_CONDITION_GUARD(annotation.condition, instance);
    const auto current_instance_location{context.instance_location(annotation)};
    const auto value{
        context.annotate(current_instance_location,
                         context.resolve_value(annotation.value, instance))};

    // As a safety guard, only emit the annotation if it didn't exist already.
    // Otherwise we risk confusing consumers
    if (value.second) {
      callback(result, step, context.evaluate_path(), current_instance_location,
               instance, value.first);
    }

    context.pop();
    return result;
  } else if (std::holds_alternative<SchemaCompilerAnnotationPrivate>(step)) {
    const auto &annotation{std::get<SchemaCompilerAnnotationPrivate>(step)};
    context.push(annotation);
    EVALUATE_CONDITION_GUARD(annotation.condition, instance);
    const auto current_instance_location{context.instance_location(annotation)};
    const auto value{
        context.annotate(current_instance_location,
                         context.resolve_value(annotation.value, instance))};
    // Annotations never fail
    result = true;

    // As a safety guard, only emit the annotation if it didn't exist already.
    // Otherwise we risk confusing consumers
    if (value.second) {
      // While this is a private annotation, we still emit it on the callback
      // for implementing debugging-related tools, etc
      callback(result, step, context.evaluate_path(), current_instance_location,
               instance, value.first);
    }

    context.pop();
    return result;
  } else if (std::holds_alternative<SchemaCompilerLoopProperties>(step)) {
    const auto &loop{std::get<SchemaCompilerLoopProperties>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop.condition, instance);
    const auto value{context.resolve_value(loop.value, instance)};
    const auto &target{context.resolve_target<JSON>(loop.target, instance)};
    assert(target.is_object());
    result = true;
    for (const auto &entry : target.as_object()) {
      context.push(empty_pointer, {entry.first});
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          result = false;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            context.pop();
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_end;
          } else {
            break;
          }
        }
      }

      context.pop();
    }

  evaluate_loop_properties_end:
    // Setting the value to false means "don't report it"
    if (!value) {
      context.pop();
      return result;
    }
  } else if (std::holds_alternative<SchemaCompilerLoopKeys>(step)) {
    const auto &loop{std::get<SchemaCompilerLoopKeys>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop.condition, instance);
    assert(std::holds_alternative<SchemaCompilerValueNone>(loop.value));
    const auto &target{context.resolve_target<JSON>(loop.target, instance)};
    assert(target.is_object());
    result = true;
    context.target_type(EvaluationContext::TargetType::Key);
    for (const auto &entry : target.as_object()) {
      context.push(empty_pointer, {entry.first});
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          result = false;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            context.pop();
            goto evaluate_loop_keys_end;
          } else {
            break;
          }
        }
      }

      context.pop();
    }

  evaluate_loop_keys_end:
    context.target_type(EvaluationContext::TargetType::Value);
  } else if (std::holds_alternative<SchemaCompilerLoopItems>(step)) {
    const auto &loop{std::get<SchemaCompilerLoopItems>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop.condition, instance);
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
      context.push(empty_pointer, {static_cast<Pointer::Token::Index>(index)});
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          result = false;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            context.pop();
            // For efficiently breaking from the outer loop too
            goto evaluate_step_end;
          } else {
            break;
          }
        }
      }

      context.pop();
    }
  } else if (std::holds_alternative<SchemaCompilerLoopContains>(step)) {
    const auto &loop{std::get<SchemaCompilerLoopContains>(step)};
    context.push(loop);
    EVALUATE_CONDITION_GUARD(loop.condition, instance);
    // TODO: Later on, extend to take a min, max pair
    // to support `minContains` and `maxContains`
    assert(std::holds_alternative<SchemaCompilerValueNone>(loop.value));
    const auto &target{context.resolve_target<JSON>(loop.target, instance)};
    assert(target.is_array());
    const auto &array{target.as_array()};
    for (auto iterator = array.cbegin(); iterator != array.cend(); ++iterator) {
      const auto index{std::distance(array.cbegin(), iterator)};
      context.push(empty_pointer, {static_cast<Pointer::Token::Index>(index)});
      bool subresult{true};
      for (const auto &child : loop.children) {
        if (!evaluate_step(child, instance, mode, callback, context)) {
          subresult = false;
          break;
        }
      }

      context.pop();
      if (subresult) {
        result = subresult;
        if (mode == SchemaCompilerEvaluationMode::Fast) {
          break;
        }
      }
    }
  }

#undef EVALUATE_CONDITION_GUARD
evaluate_step_end:
  callback(result, step, context.evaluate_path(), context.instance_location(),
           instance, context.value(nullptr));
  context.pop();
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
