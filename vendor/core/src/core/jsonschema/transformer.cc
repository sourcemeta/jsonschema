#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>     // std::erase_if
#include <cassert>       // assert
#include <functional>    // std::hash
#include <sstream>       // std::ostringstream
#include <tuple>         // std::tuple
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::pair
#include <vector>        // std::vector

namespace {

struct ProcessedRuleHasher {
  auto
  operator()(const std::tuple<const sourcemeta::core::JSON *, std::string_view,
                              std::uint64_t> &value) const noexcept
      -> std::size_t {
    return std::hash<const void *>{}(std::get<0>(value)) ^
           (std::hash<std::string_view>{}(std::get<1>(value)) << 1) ^
           (std::hash<std::uint64_t>{}(std::get<2>(value)) << 2);
  }
};

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

auto check_rules(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaFrame &frame,
    const std::vector<std::tuple<
        std::unique_ptr<sourcemeta::core::SchemaTransformRule>, bool, bool>>
        &rules,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::SchemaTransformer::Callback &callback,
    const sourcemeta::core::JSON::String &exclude_keyword,
    const bool non_mutating_only) -> std::pair<bool, std::uint8_t> {
  std::unordered_set<sourcemeta::core::Pointer,
                     sourcemeta::core::Pointer::Hasher>
      visited;
  bool result{true};
  std::size_t subschema_count{0};
  std::size_t subschema_failures{0};

  for (const auto &entry : frame.locations()) {
    if (entry.second.type !=
            sourcemeta::core::SchemaFrame::LocationType::Resource &&
        entry.second.type !=
            sourcemeta::core::SchemaFrame::LocationType::Subschema) {
      continue;
    }

    const auto [visited_iterator, inserted] =
        visited.insert(sourcemeta::core::to_pointer(entry.second.pointer));
    if (!inserted) {
      continue;
    }
    const auto &entry_pointer{*visited_iterator};

    subschema_count += 1;

    const auto &current{sourcemeta::core::get(schema, entry_pointer)};
    const auto current_vocabularies{frame.vocabularies(entry.second, resolver)};

    bool subschema_failed{false};
    for (const auto &[rule, mutates, _] : rules) {
      // TODO: In this case, can we avoid framing and the entire subschema loop
      // if there will be no rules to execute that match this criteria?
      if (non_mutating_only && mutates) {
        continue;
      }

      const auto outcome{rule->check(current, schema, current_vocabularies,
                                     walker, resolver, frame, entry.second,
                                     exclude_keyword)};
      if (outcome.applies) {
        subschema_failed = true;
        callback(entry_pointer, rule->name(), rule->message(), outcome,
                 mutates);
      }
    }

    if (subschema_failed) {
      subschema_failures += 1;
      result = false;
    }
  }

  return {result,
          calculate_health_percentage(subschema_count, subschema_failures)};
}

auto analyse_frame(sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::JSON &schema,
                   const sourcemeta::core::SchemaWalker &walker,
                   const sourcemeta::core::SchemaResolver &resolver,
                   const std::string_view default_dialect,
                   const std::string_view default_id) -> void {

  // If we use the default id when there is already one, framing will duplicate
  // the locations leading to duplicate check reports
  if (!sourcemeta::core::identify(schema, resolver, default_dialect).empty()) {
    frame.analyse(schema, walker, resolver, default_dialect);
  } else {
    frame.analyse(schema, walker, resolver, default_dialect, default_id);
  }
}

} // namespace

namespace sourcemeta::core {

SchemaTransformRule::SchemaTransformRule(const std::string_view name,
                                         const std::string_view message)
    : name_{name}, message_{message} {}

auto SchemaTransformRule::operator==(const SchemaTransformRule &other) const
    -> bool {
  return this->name() == other.name();
}

auto SchemaTransformRule::name() const noexcept -> std::string_view {
  return this->name_;
}

auto SchemaTransformRule::message() const noexcept -> std::string_view {
  return this->message_;
}

auto SchemaTransformRule::transform(JSON &, const Result &) const -> void {
  throw SchemaAbortError("This rule cannot be automatically transformed");
}

auto SchemaTransformRule::rereference(const std::string_view reference,
                                      const Pointer &origin, const Pointer &,
                                      const Pointer &) const -> Pointer {
  throw SchemaBrokenReferenceError(reference, origin,
                                   "The reference broke after transformation");
}

auto SchemaTransformRule::check(
    const JSON &schema, const JSON &root, const Vocabularies &vocabularies,
    const SchemaWalker &walker, const SchemaResolver &resolver,
    const SchemaFrame &frame, const SchemaFrame::Location &location,
    const JSON::String &exclude_keyword) const -> SchemaTransformRule::Result {
  auto result{this->condition(schema, root, vocabularies, frame, location,
                              walker, resolver)};

  // Support rule exclusion
  if (result.applies && !exclude_keyword.empty() && schema.is_object()) {
    const auto *exclude_value{schema.try_at(exclude_keyword)};
    if (exclude_value != nullptr &&
        ((exclude_value->is_string() &&
          exclude_value->to_string() == this->name()) ||
         (exclude_value->is_array() &&
          exclude_value->contains(this->name())))) {
      return false;
    }
  }

  return result;
}

auto SchemaTransformer::check(const JSON &schema, const SchemaWalker &walker,
                              const SchemaResolver &resolver,
                              const SchemaTransformer::Callback &callback,
                              std::string_view default_dialect,
                              std::string_view default_id,
                              const JSON::String &exclude_keyword) const
    -> std::pair<bool, std::uint8_t> {
  SchemaFrame frame{SchemaFrame::Mode::References};
  analyse_frame(frame, schema, walker, resolver, default_dialect, default_id);
  return check_rules(schema, frame, this->rules, walker, resolver, callback,
                     exclude_keyword, false);
}

auto SchemaTransformer::apply(JSON &schema, const SchemaWalker &walker,
                              const SchemaResolver &resolver,
                              const SchemaTransformer::Callback &callback,
                              std::string_view default_dialect,
                              std::string_view default_id,
                              const JSON::String &exclude_keyword) const
    -> std::pair<bool, std::uint8_t> {
  assert(!this->rules.empty());
  std::unordered_set<std::tuple<const JSON *, std::string_view, std::uint64_t>,
                     ProcessedRuleHasher>
      processed_rules;

  SchemaFrame frame{SchemaFrame::Mode::References};

  struct PotentiallyBrokenReference {
    Pointer origin;
    JSON::String original;
    JSON::String destination;
    Pointer target_pointer;
    std::size_t target_relative_pointer;
  };

  std::vector<PotentiallyBrokenReference> potentially_broken_references;

  while (true) {
    if (frame.empty()) {
      analyse_frame(frame, schema, walker, resolver, default_dialect,
                    default_id);
    }

    std::unordered_set<Pointer, Pointer::Hasher> visited;
    bool applied{false};

    for (const auto &entry : frame.locations()) {
      if (entry.second.type != SchemaFrame::LocationType::Resource &&
          entry.second.type != SchemaFrame::LocationType::Subschema) {
        continue;
      }

      // Framing may report resource twice or more given default identifiers and
      // nested resources, risking reporting the same errors twice
      const auto [visited_iterator, inserted] =
          visited.insert(to_pointer(entry.second.pointer));
      if (!inserted) {
        continue;
      }
      const auto &entry_pointer{*visited_iterator};
      auto &current{get(schema, entry_pointer)};
      const auto current_vocabularies{
          frame.vocabularies(entry.second, resolver)};

      for (const auto &[rule, mutates, reframe_after_transform] : this->rules) {
        // Only process mutating rules in the main loop.
        // Non-mutating rules will be processed once at the end.
        if (!mutates) {
          continue;
        }

        auto outcome{rule->check(current, schema, current_vocabularies, walker,
                                 resolver, frame, entry.second,
                                 exclude_keyword)};

        if (!outcome.applies) {
          continue;
        }

        // Collect reference information BEFORE invalidating the frame.
        // We need to save this data because after the transform, the old
        // frame's views may point to invalid memory, and a new frame won't
        // have location entries for paths that no longer exist.
        potentially_broken_references.clear();
        for (const auto &reference : frame.references()) {
          const auto destination{frame.traverse(reference.second.destination)};
          if (!destination.has_value() ||
              !reference.second.fragment.has_value() ||
              !reference.second.fragment.value().starts_with('/')) {
            continue;
          }

          const auto &target{destination.value().get()};
          potentially_broken_references.push_back(
              {to_pointer(reference.first.second),
               JSON::String{reference.second.original},
               reference.second.destination, to_pointer(target.pointer),
               target.relative_pointer});
        }

        rule->transform(current, outcome);
        callback(entry_pointer, rule->name(), rule->message(), outcome, true);

        applied = true;

        if (reframe_after_transform) {
          analyse_frame(frame, schema, walker, resolver, default_dialect,
                        default_id);
        }

        const auto new_location{frame.traverse(to_weak_pointer(entry_pointer))};
        // The location should still exist after transform
        assert(new_location.has_value());

        // Get vocabularies from the new frame
        const auto new_vocabularies{
            frame.vocabularies(new_location.value().get(), resolver)};

        // The condition must always be false after applying the
        // transformation in order to avoid infinite loops
        if (rule->check(current, schema, new_vocabularies, walker, resolver,
                        frame, new_location.value().get(), exclude_keyword)
                .applies) {
          std::ostringstream error;
          error << "Rule condition holds after application: " << rule->name();
          throw std::runtime_error(error.str());
        }

        // Identify and fix broken references using the saved data
        bool references_fixed{false};
        for (const auto &saved_reference : potentially_broken_references) {
          // The destination still exists, so we don't have to do anything
          if (try_get(schema, saved_reference.target_pointer)) {
            continue;
          }

          // If the source no longer exists, we don't need to fix the reference
          if (!try_get(schema, saved_reference.origin.initial())) {
            continue;
          }

          const auto new_fragment{rule->rereference(
              saved_reference.destination, saved_reference.origin,
              saved_reference.target_pointer.slice(
                  saved_reference.target_relative_pointer),
              entry_pointer.slice(
                  new_location.value().get().relative_pointer))};

          // Note we use the base from the original reference before any
          // canonicalisation takes place so that we don't overly change
          // user's references when only fixing up their pointer fragments
          URI original{saved_reference.original};
          original.fragment(to_string(new_fragment));
          set(schema, saved_reference.origin, JSON{original.recompose()});
          references_fixed = true;
        }

        std::tuple<const JSON *, std::string_view, std::uint64_t> mark{
            &current, rule->name(),
            // Allow applying the same rule to the same location if the schema
            // has changed, which means we are still "making progress". The
            // hashing is not perfect, but its enough
            current.fast_hash()};
        if (processed_rules.contains(mark)) {
          throw SchemaTransformRuleProcessedTwiceError(rule->name(),
                                                       entry_pointer);
        }

        processed_rules.emplace(std::move(mark));

        // If we fixed references, the schema changed again, so we need to
        // invalidate the frame. Otherwise, we can reuse it for the next
        // iteration.
        if (references_fixed) {
          frame.reset();
        }

        if (references_fixed || reframe_after_transform) {
          goto core_transformer_start_again;
        }
      }
    }

  core_transformer_start_again:
    if (!applied) {
      break;
    }
  }

  if (frame.empty()) {
    analyse_frame(frame, schema, walker, resolver, default_dialect, default_id);
  }

  return check_rules(schema, frame, this->rules, walker, resolver, callback,
                     exclude_keyword, true);
}

auto SchemaTransformer::remove(const std::string_view name) -> bool {
  return std::erase_if(this->rules, [&name](const auto &entry) {
           return std::get<0>(entry)->name() == name;
         }) > 0;
}

} // namespace sourcemeta::core
