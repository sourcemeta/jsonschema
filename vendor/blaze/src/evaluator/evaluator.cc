#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/regex.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <algorithm> // std::min, std::any_of, std::find
#include <cassert>   // assert
#include <limits>    // std::numeric_limits

namespace sourcemeta::blaze {
using namespace sourcemeta::core;

inline auto resolve_target(const JSON::String *property_target,
                           const JSON &instance) noexcept -> const JSON & {
  if (property_target) [[unlikely]] {
    // In this case, we still need to return a string in order
    // to cope with non-string keywords inside `propertyNames`
    // that need to fail validation. But then, the actual string
    // we return doesn't matter, so we can always return a dummy one.
    return Evaluator::empty_string;
  }

  return instance;
}

inline auto
resolve_string_target(const JSON::String *property_target, const JSON &instance,
                      const Pointer &relative_instance_location) noexcept
    -> const JSON::String * {
  if (property_target) [[unlikely]] {
    return property_target;
  }

  const auto &target{get(instance, relative_instance_location)};
  if (!target.is_string()) {
    return nullptr;
  } else {
    return &target.to_string();
  }
}

} // namespace sourcemeta::blaze

#define SOURCEMETA_STRINGIFY(x) #x
#define SOURCEMETA_MAYBE_UNUSED(variable) (void)variable;

#include "evaluator_complete.h"
#include "evaluator_dynamic.h"
#include "evaluator_fast.h"
#include "evaluator_track.h"

#undef SOURCEMETA_STRINGIFY
#undef SOURCEMETA_MAYBE_UNUSED

namespace sourcemeta::blaze {

auto Evaluator::validate(const Template &schema,
                         const sourcemeta::core::JSON &instance) -> bool {
  // Do a full reset for the next run
  assert(this->evaluate_path.empty());
  assert(this->instance_location.empty());
  assert(this->resources.empty());
  this->labels.clear();

  if (schema.track && schema.dynamic) {
    this->evaluated_.clear();
    return complete::evaluate(instance, *this, schema, nullptr);
  } else if (schema.track) {
    this->evaluated_.clear();
    return track::evaluate(instance, *this, schema);
  } else if (schema.dynamic) {
    return dynamic::evaluate(instance, *this, schema);
  } else {
    return fast::evaluate(instance, *this, schema);
  }
}

auto Evaluator::validate(const Template &schema,
                         const sourcemeta::core::JSON &instance,
                         const Callback &callback) -> bool {
  // Do a full reset for the next run
  assert(this->evaluate_path.empty());
  assert(this->instance_location.empty());
  assert(this->resources.empty());
  this->labels.clear();
  this->evaluated_.clear();

  return complete::evaluate(instance, *this, schema, callback);
}

const sourcemeta::core::JSON Evaluator::null{nullptr};
const sourcemeta::core::JSON Evaluator::empty_string{""};

auto Evaluator::hash(const std::size_t resource,
                     const sourcemeta::core::JSON::String &fragment)
    const noexcept -> std::size_t {
  return resource + this->hasher_(fragment);
}

auto Evaluator::evaluate(const sourcemeta::core::JSON *target) -> void {
  Evaluation mark{target, this->evaluate_path, false};
  this->evaluated_.push_back(std::move(mark));
}

auto Evaluator::is_evaluated(const sourcemeta::core::JSON *target) const
    -> bool {
  for (auto iterator = this->evaluated_.crbegin();
       iterator != this->evaluated_.crend(); ++iterator) {
    if (target == iterator->instance && !iterator->skip &&
        // Its not possible to affect cousins
        iterator->evaluate_path.starts_with_initial(this->evaluate_path)) {
      return true;
    }
  }

  return false;
}

auto Evaluator::unevaluate() -> void {
  for (auto &entry : this->evaluated_) {
    if (!entry.skip && entry.evaluate_path.starts_with(this->evaluate_path)) {
      entry.skip = true;
    }
  }
}

} // namespace sourcemeta::blaze
