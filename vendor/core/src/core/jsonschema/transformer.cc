#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <cassert>   // assert
#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error
#include <utility>   // std::move, std::pair

namespace {

auto calculate_health_percentage(const std::size_t subschemas,
                                 const std::size_t failed_subschemas)
    -> std::uint8_t {
  assert(failed_subschemas <= subschemas);
  const auto result{100 - (failed_subschemas * 100 / subschemas)};
  assert(result <= 100);
  return static_cast<std::uint8_t>(result);
}

} // namespace

namespace sourcemeta::core {

SchemaTransformRule::SchemaTransformRule(std::string &&name,
                                         std::string &&message)
    : name_{std::move(name)}, message_{std::move(message)} {}

auto SchemaTransformRule::operator==(const SchemaTransformRule &other) const
    -> bool {
  return this->name() == other.name();
}

auto SchemaTransformRule::name() const -> const std::string & {
  return this->name_;
}

auto SchemaTransformRule::message() const -> const std::string & {
  return this->message_;
}

auto SchemaTransformRule::transform(JSON &, const Result &) const -> void {
  throw SchemaAbortError("This rule cannot be automatically transformed");
}

auto SchemaTransformRule::rereference(const std::string &reference,
                                      const Pointer &origin, const Pointer &,
                                      const Pointer &) const -> Pointer {
  throw SchemaBrokenReferenceError(reference, origin,
                                   "The reference broke after transformation");
}

auto SchemaTransformRule::apply(JSON &schema, const JSON &root,
                                const Vocabularies &vocabularies,
                                const SchemaWalker &walker,
                                const SchemaResolver &resolver,
                                const SchemaFrame &frame,
                                const SchemaFrame::Location &location) const
    -> std::pair<bool, Result> {
  auto outcome{this->condition(schema, root, vocabularies, frame, location,
                               walker, resolver)};
  if (!outcome.applies) {
    return {true, std::move(outcome)};
  }

  try {
    this->transform(schema, outcome);
  } catch (const SchemaAbortError &) {
    return {false, std::move(outcome)};
  }

  // The condition must always be false after applying the
  // transformation in order to avoid infinite loops
  if (this->condition(schema, root, vocabularies, frame, location, walker,
                      resolver)
          .applies) {
    // TODO: Throw a better custom error that also highlights the schema
    // location
    std::ostringstream error;
    error << "Rule condition holds after application: " << this->name();
    throw std::runtime_error(error.str());
  }

  return {true, std::move(outcome)};
}

auto SchemaTransformRule::check(const JSON &schema, const JSON &root,
                                const Vocabularies &vocabularies,
                                const SchemaWalker &walker,
                                const SchemaResolver &resolver,
                                const SchemaFrame &frame,
                                const SchemaFrame::Location &location) const
    -> SchemaTransformRule::Result {
  return this->condition(schema, root, vocabularies, frame, location, walker,
                         resolver);
}

auto SchemaTransformer::check(
    const JSON &schema, const SchemaWalker &walker,
    const SchemaResolver &resolver, const SchemaTransformer::Callback &callback,
    const std::optional<JSON::String> &default_dialect,
    const std::optional<JSON::String> &default_id) const
    -> std::pair<bool, std::uint8_t> {
  SchemaFrame frame{SchemaFrame::Mode::Locations};
  frame.analyse(schema, walker, resolver, default_dialect, default_id);

  bool result{true};
  std::size_t subschema_count{0};
  std::size_t subschema_failures{0};
  for (const auto &entry : frame.locations()) {
    if (entry.second.type != SchemaFrame::LocationType::Resource &&
        entry.second.type != SchemaFrame::LocationType::Subschema) {
      continue;
    }

    subschema_count += 1;

    const auto &current{get(schema, entry.second.pointer)};
    const auto current_vocabularies{
        vocabularies(schema, resolver, entry.second.dialect)};
    bool subresult{true};
    for (const auto &[name, rule] : this->rules) {
      const auto outcome{rule->check(current, schema, current_vocabularies,
                                     walker, resolver, frame, entry.second)};
      if (outcome.applies) {
        subresult = false;
        callback(entry.second.pointer, name, rule->message(), outcome);
      }
    }

    if (!subresult) {
      subschema_failures += 1;
      result = false;
    }
  }

  return {result,
          calculate_health_percentage(subschema_count, subschema_failures)};
}

auto SchemaTransformer::apply(
    JSON &schema, const SchemaWalker &walker, const SchemaResolver &resolver,
    const SchemaTransformer::Callback &callback,
    const std::optional<JSON::String> &default_dialect,
    const std::optional<JSON::String> &default_id) const -> bool {
  // There is no point in applying an empty bundle
  assert(!this->rules.empty());
  std::set<std::pair<const JSON *, const JSON::String *>> processed_rules;

  bool result{true};
  while (true) {
    SchemaFrame frame{SchemaFrame::Mode::References};
    frame.analyse(schema, walker, resolver, default_dialect, default_id);

    bool applied{false};
    for (const auto &entry : frame.locations()) {
      if (entry.second.type != SchemaFrame::LocationType::Resource &&
          entry.second.type != SchemaFrame::LocationType::Subschema) {
        continue;
      }

      auto &current{get(schema, entry.second.pointer)};
      const auto current_vocabularies{
          vocabularies(schema, resolver, entry.second.dialect)};
      for (const auto &[name, rule] : this->rules) {
        const auto subresult{rule->apply(current, schema, current_vocabularies,
                                         walker, resolver, frame,
                                         entry.second)};
        // This means the rule is fixable
        if (subresult.first) {
          applied = subresult.second.applies || applied;
        } else {
          result = false;
          callback(entry.second.pointer, name, rule->message(),
                   subresult.second);
        }

        if (!applied) {
          continue;
        }

        std::pair<const JSON *, const JSON::String *> mark{&current, &name};
        if (processed_rules.contains(mark)) {
          // TODO: Throw a better custom error that also highlights the schema
          // location
          std::ostringstream error;
          error << "Rules must only be processed once: " << name;
          throw std::runtime_error(error.str());
        }

        // Identify and try to address broken references, if any
        for (const auto &reference : frame.references()) {
          const auto destination{frame.traverse(reference.second.destination)};
          if (!destination.has_value() ||
              // We only care about references with JSON Pointer fragments,
              // as these are the only cases, by definition, where the target
              // is location-dependent.
              !reference.second.fragment.has_value() ||
              !reference.second.fragment.value().starts_with('/')) {
            continue;
          }

          const auto &target{destination.value().get().pointer};
          // The destination still exists, so we don't have to do anything
          if (try_get(schema, target)) {
            continue;
          }

          const auto new_fragment{rule->rereference(
              reference.second.destination, reference.first.second, target,
              entry.second.pointer)};

          // Note we use the base from the original reference before any
          // canonicalisation takes place so that we don't overly change
          // user's references when only fixing up their pointer fragments
          URI original{reference.second.original};
          original.fragment(to_string(new_fragment));
          set(schema, reference.first.second, JSON{original.recompose()});
        }

        processed_rules.emplace(std::move(mark));
        goto core_transformer_start_again;
      }
    }

  core_transformer_start_again:
    if (!applied) {
      break;
    }
  }

  return result;
}

auto SchemaTransformer::remove(const std::string &name) -> bool {
  return this->rules.erase(name) > 0;
}

} // namespace sourcemeta::core
