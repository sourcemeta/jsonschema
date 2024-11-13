#include <sourcemeta/blaze/evaluator_context.h>
#include <sourcemeta/blaze/evaluator_error.h>

#include <cassert> // assert

namespace sourcemeta::blaze {

auto EvaluationContext::prepare(const sourcemeta::jsontoolkit::JSON &instance)
    -> void {
  // Do a full reset for the next run
  assert(this->evaluate_path.empty());
  assert(this->evaluate_path_size == 0);
  assert(this->instance_location.empty());
  assert(!this->property_target.has_value());
  assert(this->frame_sizes.empty());
  assert(this->resources.empty());
  this->instances.clear();
  this->instances.emplace_back(instance);
  this->labels.clear();
  this->evaluated_.clear();
}

auto EvaluationContext::push_without_traverse(
    const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
    const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
    const std::size_t &schema_resource, const bool dynamic, const bool track)
    -> void {
  // Guard against infinite recursion in a cheap manner, as
  // infinite recursion will manifest itself through huge
  // ever-growing evaluate paths
  constexpr auto EVALUATE_PATH_LIMIT{300};
  const auto stack_size{track ? this->evaluate_path.size()
                              : this->evaluate_path_size};
  if (stack_size > EVALUATE_PATH_LIMIT) [[unlikely]] {
    throw sourcemeta::blaze::EvaluationError(
        "The evaluation path depth limit was reached "
        "likely due to infinite recursion");
  }

  this->frame_sizes.emplace_back(relative_schema_location.size(),
                                 relative_instance_location.size());

  if (track) {
    this->evaluate_path.push_back(relative_schema_location);
    this->instance_location.push_back(relative_instance_location);
  } else {
    // We still need to somewhat keep track of this to prevent infinite
    // recursion
    this->evaluate_path_size += relative_schema_location.size();
  }

  if (dynamic) {
    // Note that we are potentially repeatedly pushing back the
    // same schema resource over and over again. However, the
    // logic for making sure this list is "pure" takes a lot of
    // computation power. Being silly seems faster.
    this->resources.push_back(schema_resource);
  }
}

auto EvaluationContext::push(
    const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
    const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
    const std::size_t &schema_resource, const bool dynamic, const bool track)
    -> void {
  this->push_without_traverse(relative_schema_location,
                              relative_instance_location, schema_resource,
                              dynamic, track);
  if (!relative_instance_location.empty()) {
    assert(!this->instances.empty());
    this->instances.emplace_back(
        get(this->instances.back().get(), relative_instance_location));
  }
}

auto EvaluationContext::push(
    const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
    const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
    const std::size_t &schema_resource, const bool dynamic, const bool track,
    std::reference_wrapper<const sourcemeta::jsontoolkit::JSON> &&new_instance)
    -> void {
  this->push_without_traverse(relative_schema_location,
                              relative_instance_location, schema_resource,
                              dynamic, track);
  assert(!relative_instance_location.empty());
  this->instances.emplace_back(new_instance);
}

auto EvaluationContext::pop(const bool dynamic, const bool track) -> void {
  assert(!this->frame_sizes.empty());
  const auto &sizes{this->frame_sizes.back()};
  if (sizes.second > 0) {
    this->instances.pop_back();
  }

  if (track) {
    this->evaluate_path.pop_back(sizes.first);
    if (sizes.second > 0) {
      this->instance_location.pop_back(sizes.second);
    }
  } else {
    this->evaluate_path_size -= sizes.first;
  }

  this->frame_sizes.pop_back();

  if (dynamic) {
    assert(!this->resources.empty());
    this->resources.pop_back();
  }
}

auto EvaluationContext::enter(
    const sourcemeta::jsontoolkit::WeakPointer::Token::Property &property,
    const bool track) -> void {
  if (track) {
    this->instance_location.push_back(property);
  }

  this->instances.emplace_back(this->instances.back().get().at(property));
}

auto EvaluationContext::enter(
    const sourcemeta::jsontoolkit::WeakPointer::Token::Index &index,
    const bool track) -> void {
  if (track) {
    this->instance_location.push_back(index);
  }

  this->instances.emplace_back(this->instances.back().get().at(index));
}

auto EvaluationContext::leave(const bool track) -> void {
  if (track) {
    this->instance_location.pop_back();
  }

  this->instances.pop_back();
}

auto EvaluationContext::hash(const std::size_t &resource,
                             const std::string &fragment) const noexcept
    -> std::size_t {
  return resource + this->hasher_(fragment);
}

auto EvaluationContext::resolve_target()
    -> const sourcemeta::jsontoolkit::JSON & {
  if (this->property_target.has_value()) [[unlikely]] {
    // In this case, we still need to return a string in order
    // to cope with non-string keywords inside `propertyNames`
    // that need to fail validation. But then, the actual string
    // we return doesn't matter, so we can always return a dummy one.
    static const sourcemeta::jsontoolkit::JSON empty_string{""};
    return empty_string;
  }

  return this->instances.back().get();
}

auto EvaluationContext::resolve_string_target() -> std::optional<
    std::reference_wrapper<const sourcemeta::jsontoolkit::JSON::String>> {
  if (this->property_target.has_value()) [[unlikely]] {
    return this->property_target.value();
  } else {
    const auto &result{this->instances.back().get()};
    if (!result.is_string()) {
      return std::nullopt;
    }

    return result.to_string();
  }
}

auto EvaluationContext::evaluate() -> void {
  this->evaluate(sourcemeta::jsontoolkit::empty_pointer);
}

auto EvaluationContext::evaluate(
    const sourcemeta::jsontoolkit::WeakPointer::Token::Index from,
    const sourcemeta::jsontoolkit::WeakPointer::Token::Index to) -> void {
  for (auto cursor = from; cursor <= to; cursor++) {
    this->evaluate({cursor});
  }
}

auto EvaluationContext::evaluate(
    const sourcemeta::jsontoolkit::Pointer &relative_instance_location)
    -> void {
  auto new_instance_location = this->instance_location;
  new_instance_location.push_back(relative_instance_location);
  Evaluation entry{std::move(new_instance_location), this->evaluate_path,
                   false};
  this->evaluated_.emplace_back(std::move(entry));
}

auto EvaluationContext::is_evaluated(
    const sourcemeta::jsontoolkit::WeakPointer::Token &tail) const -> bool {
  for (auto iterator = this->evaluated_.crbegin();
       iterator != this->evaluated_.crend(); ++iterator) {
    if (!iterator->skip &&
        this->instance_location.starts_with(iterator->instance_location,
                                            tail) &&
        // Its not possible to affect cousins
        iterator->evaluate_path.starts_with_initial(this->evaluate_path)) {
      return true;
    }
  }

  return false;
}

auto EvaluationContext::unevaluate() -> void {
  for (auto &entry : this->evaluated_) {
    if (!entry.skip && entry.evaluate_path.starts_with(this->evaluate_path)) {
      entry.skip = true;
    }
  }
}

} // namespace sourcemeta::blaze
