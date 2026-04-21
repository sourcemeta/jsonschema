#include <sourcemeta/blaze/alterschema.h>
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
  operator()(const std::tuple<sourcemeta::core::Pointer, std::string_view,
                              sourcemeta::core::JSON> &value) const noexcept
      -> std::size_t {
    return sourcemeta::core::Pointer::Hasher{}(std::get<0>(value)) ^
           (std::hash<std::string_view>{}(std::get<1>(value)) << 1) ^
           (std::hash<std::uint64_t>{}(std::get<2>(value).fast_hash()) << 2);
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
        std::unique_ptr<sourcemeta::blaze::SchemaTransformRule>, bool, bool>>
        &rules,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::blaze::SchemaTransformer::Callback &callback,
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
  if (!sourcemeta::core::identify(schema, resolver, default_dialect).empty()) {
    frame.analyse(schema, walker, resolver, default_dialect);
  } else {
    frame.analyse(schema, walker, resolver, default_dialect, default_id);
  }
}

} // namespace

namespace sourcemeta::blaze {

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

auto SchemaTransformRule::transform(core::JSON &, const Result &) const
    -> void {
  throw SchemaAbortError("This rule cannot be automatically transformed");
}

auto SchemaTransformRule::rereference(const std::string_view reference,
                                      const core::Pointer &origin,
                                      const core::Pointer &,
                                      const core::Pointer &) const
    -> core::Pointer {
  throw SchemaBrokenReferenceError(reference, origin,
                                   "The reference broke after transformation");
}

auto SchemaTransformRule::check(const core::JSON &schema,
                                const core::JSON &root,
                                const core::Vocabularies &vocabularies,
                                const core::SchemaWalker &walker,
                                const core::SchemaResolver &resolver,
                                const core::SchemaFrame &frame,
                                const core::SchemaFrame::Location &location,
                                const core::JSON::String &exclude_keyword) const
    -> SchemaTransformRule::Result {
  auto result{this->condition(schema, root, vocabularies, frame, location,
                              walker, resolver)};

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

auto SchemaTransformer::check(const core::JSON &schema,
                              const core::SchemaWalker &walker,
                              const core::SchemaResolver &resolver,
                              const SchemaTransformer::Callback &callback,
                              std::string_view default_dialect,
                              std::string_view default_id,
                              const core::JSON::String &exclude_keyword) const
    -> std::pair<bool, std::uint8_t> {
  core::SchemaFrame frame{core::SchemaFrame::Mode::References};
  analyse_frame(frame, schema, walker, resolver, default_dialect, default_id);
  return check_rules(schema, frame, this->rules, walker, resolver, callback,
                     exclude_keyword, false);
}

auto SchemaTransformer::apply(core::JSON &schema,
                              const core::SchemaWalker &walker,
                              const core::SchemaResolver &resolver,
                              const SchemaTransformer::Callback &callback,
                              std::string_view default_dialect,
                              std::string_view default_id,
                              const core::JSON::String &exclude_keyword) const
    -> std::pair<bool, std::uint8_t> {
  assert(!this->rules.empty());
  std::unordered_set<std::tuple<core::Pointer, std::string_view, core::JSON>,
                     ProcessedRuleHasher>
      processed_rules;

  core::SchemaFrame frame{core::SchemaFrame::Mode::References};

  struct PotentiallyBrokenReference {
    core::Pointer origin;
    core::JSON::String original;
    core::JSON::String destination;
    core::JSON::String fragment;
    core::Pointer target_pointer;
    std::size_t target_relative_pointer;
  };

  std::vector<PotentiallyBrokenReference> potentially_broken_references;

  while (true) {
    if (frame.empty()) {
      if (schema.is_boolean()) {
        break;
      }

      analyse_frame(frame, schema, walker, resolver, default_dialect,
                    default_id);
    }

    std::unordered_set<core::Pointer, core::Pointer::Hasher> visited;
    bool applied{false};

    for (const auto &entry : frame.locations()) {
      if (entry.second.type != core::SchemaFrame::LocationType::Resource &&
          entry.second.type != core::SchemaFrame::LocationType::Subschema) {
        continue;
      }

      const auto [visited_iterator, inserted] =
          visited.insert(core::to_pointer(entry.second.pointer));
      if (!inserted) {
        continue;
      }
      const auto &entry_pointer{*visited_iterator};
      auto &current{core::get(schema, entry_pointer)};
      const auto current_vocabularies{
          frame.vocabularies(entry.second, resolver)};

      for (const auto &[rule, mutates, reframe_after_transform] : this->rules) {
        if (!mutates) {
          continue;
        }

        auto outcome{rule->check(current, schema, current_vocabularies, walker,
                                 resolver, frame, entry.second,
                                 exclude_keyword)};

        if (!outcome.applies) {
          continue;
        }

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
              {core::to_pointer(reference.first.second),
               core::JSON::String{reference.second.original},
               reference.second.destination,
               core::JSON::String{reference.second.fragment.value()},
               core::to_pointer(target.pointer), target.relative_pointer});
        }

        rule->transform(current, outcome);
        callback(entry_pointer, rule->name(), rule->message(), outcome, true);

        applied = true;

        if (reframe_after_transform) {
          analyse_frame(frame, schema, walker, resolver, default_dialect,
                        default_id);
        } else if (current.is_boolean()) {
          std::tuple<core::Pointer, std::string_view, core::JSON> mark{
              entry_pointer, rule->name(), current};
          if (processed_rules.contains(mark)) {
            throw SchemaTransformRuleProcessedTwiceError(rule->name(),
                                                         entry_pointer);
          }

          processed_rules.emplace(std::move(mark));
          frame.reset();
          goto blaze_transformer_start_again;
        }

        const auto new_location{
            frame.traverse(core::to_weak_pointer(entry_pointer))};
        assert(new_location.has_value());

        const auto new_vocabularies{
            frame.vocabularies(new_location.value().get(), resolver)};

        if (rule->check(current, schema, new_vocabularies, walker, resolver,
                        frame, new_location.value().get(), exclude_keyword)
                .applies) {
          std::ostringstream error;
          error << "Rule condition holds after application: " << rule->name();
          throw std::runtime_error(error.str());
        }

        bool references_fixed{false};
        for (const auto &saved_reference : potentially_broken_references) {
          if (core::try_get(schema, saved_reference.target_pointer)) {
            continue;
          }

          if (!core::try_get(schema, saved_reference.origin.initial())) {
            continue;
          }

          const auto new_relative{rule->rereference(
              saved_reference.destination, saved_reference.origin,
              saved_reference.target_pointer.slice(
                  saved_reference.target_relative_pointer),
              entry_pointer.slice(
                  new_location.value().get().relative_pointer))};
          const auto new_fragment{
              saved_reference.fragment ==
                      core::to_string(saved_reference.target_pointer)
                  ? saved_reference.target_pointer
                        .slice(0, saved_reference.target_relative_pointer)
                        .concat(new_relative)
                  : new_relative};

          core::URI original{saved_reference.original};
          original.fragment(core::to_string(new_fragment));
          core::set(schema, saved_reference.origin,
                    core::JSON{original.recompose()});
          references_fixed = true;
        }

        std::tuple<core::Pointer, std::string_view, core::JSON> mark{
            entry_pointer, rule->name(), current};
        if (processed_rules.contains(mark)) {
          throw SchemaTransformRuleProcessedTwiceError(rule->name(),
                                                       entry_pointer);
        }

        processed_rules.emplace(std::move(mark));

        if (references_fixed) {
          frame.reset();
        }

        if (references_fixed || reframe_after_transform) {
          goto blaze_transformer_start_again;
        }
      }
    }

  blaze_transformer_start_again:
    if (!applied) {
      break;
    }
  }

  if (frame.empty() && !schema.is_boolean()) {
    analyse_frame(frame, schema, walker, resolver, default_dialect, default_id);
  }

  if (frame.empty()) {
    return {true, static_cast<std::uint8_t>(100)};
  }

  return check_rules(schema, frame, this->rules, walker, resolver, callback,
                     exclude_keyword, true);
}

auto SchemaTransformer::remove(const std::string_view name) -> bool {
  return std::erase_if(this->rules, [&name](const auto &entry) {
           return std::get<0>(entry)->name() == name;
         }) > 0;
}

} // namespace sourcemeta::blaze
