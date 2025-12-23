#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>     // std::erase_if
#include <cassert>       // assert
#include <set>           // std::set
#include <sstream>       // std::ostringstream
#include <tuple>         // std::tuple
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::pair

namespace {

auto calculate_health_percentage(const std::size_t subschemas,
                                 const std::size_t failed_subschemas)
    -> std::uint8_t {
  assert(failed_subschemas <= subschemas);
  if (subschemas == 0) {
    return 100;
  }

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
  SchemaFrame frame{SchemaFrame::Mode::Instances};

  // If we use the default id when there is already one, framing will duplicate
  // the locations leading to duplicate check reports
  if (sourcemeta::core::identify(schema, resolver, default_dialect)
          .has_value()) {
    frame.analyse(schema, walker, resolver, default_dialect);
  } else {
    frame.analyse(schema, walker, resolver, default_dialect, default_id);
  }

  std::unordered_set<Pointer> visited;
  bool result{true};
  std::size_t subschema_count{0};
  std::size_t subschema_failures{0};
  for (const auto &entry : frame.locations()) {
    if (entry.second.type != SchemaFrame::LocationType::Resource &&
        entry.second.type != SchemaFrame::LocationType::Subschema) {
      continue;
    }

    // Framing may report resource twice or more given default identifiers and
    // nested resources, risking reporting the same errors twice
    if (!visited.insert(entry.second.pointer).second) {
      continue;
    }

    subschema_count += 1;

    const auto &current{get(schema, entry.second.pointer)};
    const auto current_vocabularies{frame.vocabularies(entry.second, resolver)};
    bool subresult{true};
    for (const auto &rule : this->rules) {
      const auto outcome{rule->check(current, schema, current_vocabularies,
                                     walker, resolver, frame, entry.second)};
      if (outcome.applies) {
        subresult = false;
        callback(entry.second.pointer, rule->name(), rule->message(), outcome);
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
    const std::optional<JSON::String> &default_id) const
    -> std::pair<bool, std::uint8_t> {
  // There is no point in applying an empty bundle
  assert(!this->rules.empty());
  std::set<std::tuple<const JSON *, const JSON::String *, std::uint64_t>>
      processed_rules;

  bool result{true};
  std::size_t subschema_count{0};
  std::size_t subschema_failures{0};
  while (true) {
    SchemaFrame frame{SchemaFrame::Mode::Instances};
    frame.analyse(schema, walker, resolver, default_dialect, default_id);
    std::unordered_set<Pointer> visited;

    bool applied{false};
    subschema_count = 0;
    subschema_failures = 0;
    for (const auto &entry : frame.locations()) {
      if (entry.second.type != SchemaFrame::LocationType::Resource &&
          entry.second.type != SchemaFrame::LocationType::Subschema) {
        continue;
      }

      // Framing may report resource twice or more given default identifiers and
      // nested resources, risking reporting the same errors twice
      if (!visited.insert(entry.second.pointer).second) {
        continue;
      }

      subschema_count += 1;

      auto &current{get(schema, entry.second.pointer)};
      const auto current_vocabularies{
          frame.vocabularies(entry.second, resolver)};

      bool subschema_failed{false};
      for (const auto &rule : this->rules) {
        const auto subresult{rule->apply(current, schema, current_vocabularies,
                                         walker, resolver, frame,
                                         entry.second)};
        // This means the rule is fixable
        if (subresult.first) {
          applied = subresult.second.applies || applied;
        } else {
          result = false;
          subschema_failed = true;
          callback(entry.second.pointer, rule->name(), rule->message(),
                   subresult.second);
        }

        if (!applied) {
          continue;
        }

        std::tuple<const JSON *, const JSON::String *, std::uint64_t> mark{
            &current, &rule->name(),
            // Allow applying the same rule to the same location if the schema
            // has changed, which means we are still "making progress". The
            // hashing is not perfect, but its enough
            current.fast_hash()};
        if (processed_rules.contains(mark)) {
          throw SchemaTransformRuleProcessedTwiceError(rule->name(),
                                                       entry.second.pointer);
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

          const auto &target{destination.value().get()};
          // The destination still exists, so we don't have to do anything
          if (try_get(schema, target.pointer)) {
            continue;
          }

          // If the source no longer exists, we don't need to fix the reference
          if (!try_get(schema, reference.first.second.initial())) {
            continue;
          }

          const auto new_fragment{rule->rereference(
              reference.second.destination, reference.first.second,
              target.relative_pointer, entry.second.relative_pointer)};

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

      if (subschema_failed) {
        subschema_failures += 1;
      }
    }

  core_transformer_start_again:
    if (!applied) {
      break;
    }
  }

  return {result,
          calculate_health_percentage(subschema_count, subschema_failures)};
}

auto SchemaTransformer::remove(const std::string &name) -> bool {
  return std::erase_if(this->rules, [&name](const auto &rule) {
           return rule->name() == name;
         }) > 0;
}

} // namespace sourcemeta::core
