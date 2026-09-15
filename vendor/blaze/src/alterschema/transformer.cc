#include <sourcemeta/blaze/alterschema.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>     // std::erase_if
#include <cassert>       // assert
#include <functional>    // std::hash
#include <optional>      // std::optional
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

auto reframe(const sourcemeta::blaze::SchemaTransformer::Framer &framer,
             const sourcemeta::core::JSON &document)
    -> const sourcemeta::core::SchemaFrame * {
  const auto &frame{framer(document)};
  assert(frame.mode() == sourcemeta::core::SchemaFrame::Mode::References);
  return &frame;
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

  frame.for_each_subschema(

      [&](const sourcemeta::core::SchemaFrame::Location &location) -> void {
        const auto [visited_iterator, inserted] =
            visited.insert(sourcemeta::core::to_pointer(location.pointer));
        if (!inserted) {
          return;
        }
        const auto &entry_pointer{*visited_iterator};

        subschema_count += 1;

        const auto &current{sourcemeta::core::get(schema, entry_pointer)};
        const auto &current_vocabularies{
            frame.vocabularies(location, resolver)};

        bool subschema_failed{false};
        for (const auto &[rule, mutates, reframe_after_transform] : rules) {
          if (non_mutating_only && mutates) {
            continue;
          }

          const auto outcome{rule->check(current, schema, current_vocabularies,
                                         walker, resolver, frame, location,
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
      });

  return {result,
          calculate_health_percentage(subschema_count, subschema_failures)};
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
                                const core::SchemaVocabularies &vocabularies,
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
  const core::SchemaFrame frame{
      core::SchemaFrame::Mode::References,
      schema,
      walker,
      resolver,
      default_dialect,
      default_id,
      sourcemeta::core::SchemaFrame::IdentifierMode::Fallback};
  return this->check(schema, frame, walker, resolver, callback,
                     exclude_keyword);
}

auto SchemaTransformer::check(const core::JSON &document,
                              const core::SchemaFrame &frame,
                              const core::SchemaWalker &walker,
                              const core::SchemaResolver &resolver,
                              const SchemaTransformer::Callback &callback,
                              const core::JSON::String &exclude_keyword) const
    -> std::pair<bool, std::uint8_t> {
  assert(frame.mode() == core::SchemaFrame::Mode::References);
  return check_rules(document, frame, this->rules_, walker, resolver, callback,
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
  std::optional<core::SchemaFrame> frame;
  return this->apply(
      schema,
      [&frame, &walker, &resolver, default_dialect,
       default_id](const core::JSON &document) -> const core::SchemaFrame & {
        frame.emplace(core::SchemaFrame::Mode::References, document, walker,
                      resolver, default_dialect, default_id,
                      sourcemeta::core::SchemaFrame::IdentifierMode::Fallback);
        return frame.value();
      },
      walker, resolver, callback, exclude_keyword);
}

auto SchemaTransformer::apply(core::JSON &document, const Framer &framer,
                              const core::SchemaWalker &walker,
                              const core::SchemaResolver &resolver,
                              const SchemaTransformer::Callback &callback,
                              const core::JSON::String &exclude_keyword) const
    -> std::pair<bool, std::uint8_t> {
  assert(!this->rules_.empty());
  std::unordered_set<std::tuple<core::Pointer, std::string_view, core::JSON>,
                     ProcessedRuleHasher>
      processed_rules;

  const core::SchemaFrame *frame{nullptr};

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
    if (frame == nullptr) {
      if (document.is_boolean()) {
        break;
      }

      frame = reframe(framer, document);
    }

    std::unordered_set<core::Pointer, core::Pointer::Hasher> visited;
    bool applied{false};

    // Stopping the traversal stands in for the restart that the
    // rules request once they mutate the schema
    [[maybe_unused]] const auto restarted{frame->any_subschema(
        [&](const core::SchemaFrame::Location &location) -> bool {
          const auto [visited_iterator, inserted] =
              visited.insert(core::to_pointer(location.pointer));
          if (!inserted) {
            return false;
          }
          const auto &entry_pointer{*visited_iterator};
          auto &current{core::get(document, entry_pointer)};
          const auto current_vocabularies{
              frame->vocabularies(location, resolver)};

          for (const auto &[rule, mutates, reframe_after_transform] :
               this->rules_) {
            if (!mutates) {
              continue;
            }

            auto outcome{rule->check(current, document, current_vocabularies,
                                     walker, resolver, *frame, location,
                                     exclude_keyword)};

            if (!outcome.applies) {
              continue;
            }

            potentially_broken_references.clear();
            frame->for_each_reference([&](const core::SchemaReferenceType,
                                          const core::WeakPointer &origin,
                                          const core::SchemaFrame::Reference
                                              &reference) -> void {
              const auto destination{frame->traverse(reference.destination)};
              if (!destination.has_value() || !reference.fragment.has_value() ||
                  !reference.fragment.value().starts_with('/')) {
                return;
              }

              const auto &target{destination.value().get()};
              potentially_broken_references.push_back(
                  {.origin = core::to_pointer(origin),
                   .original = core::JSON::String{reference.original},
                   .destination = reference.destination,
                   .fragment = core::JSON::String{reference.fragment.value()},
                   .target_pointer = core::to_pointer(target.pointer),
                   .target_relative_pointer = target.relative_pointer});
            });

            rule->transform(current, outcome);
            callback(entry_pointer, rule->name(), rule->message(), outcome,
                     true);

            applied = true;

            if (reframe_after_transform) {
              frame = reframe(framer, document);
            } else if (current.is_boolean()) {
              std::tuple<core::Pointer, std::string_view, core::JSON> mark{
                  entry_pointer, rule->name(), current};
              if (processed_rules.contains(mark)) {
                throw SchemaTransformRuleProcessedTwiceError(rule->name(),
                                                             entry_pointer);
              }

              processed_rules.emplace(std::move(mark));
              frame = nullptr;
              return true;
            }

            const auto new_location{
                frame->traverse(core::to_weak_pointer(entry_pointer))};
            assert(new_location.has_value());

            // Fix broken references before re-checking the condition,
            // as the re-check may mutate rule state that rereference needs
            bool references_fixed{false};
            const auto resource_offset{
                new_location.value().get().relative_pointer};
            const auto current_slice{entry_pointer.slice(resource_offset)};
            for (const auto &saved_reference : potentially_broken_references) {
              if (core::try_get(document, saved_reference.target_pointer)) {
                continue;
              }

              // If the origin was also relocated, resolve its new location
              auto effective_origin{saved_reference.origin};
              if (!core::try_get(document, saved_reference.origin.initial())) {
                try {
                  const auto new_origin{rule->rereference(
                      saved_reference.destination, saved_reference.origin,
                      saved_reference.origin.slice(resource_offset),
                      current_slice)};
                  effective_origin =
                      saved_reference.origin.slice(0, resource_offset)
                          .concat(new_origin);
                } catch (...) {
                  continue;
                }
                if (!core::try_get(document, effective_origin.initial())) {
                  continue;
                }
              }

              const auto new_relative{rule->rereference(
                  saved_reference.destination, saved_reference.origin,
                  saved_reference.target_pointer.slice(
                      saved_reference.target_relative_pointer),
                  current_slice)};
              const auto new_fragment{
                  saved_reference.fragment ==
                          core::to_string(saved_reference.target_pointer)
                      ? saved_reference.target_pointer
                            .slice(0, saved_reference.target_relative_pointer)
                            .concat(new_relative)
                      : new_relative};

              core::URI original{saved_reference.original};
              original.fragment(core::to_string(new_fragment));
              core::set(document, effective_origin,
                        core::JSON{original.recompose()});
              references_fixed = true;
            }

            const auto new_vocabularies{
                frame->vocabularies(new_location.value().get(), resolver)};

            if (rule->check(current, document, new_vocabularies, walker,
                            resolver, *frame, new_location.value().get(),
                            exclude_keyword)
                    .applies) {
              std::ostringstream error;
              error << "Rule condition holds after application: "
                    << rule->name();
              throw std::runtime_error(error.str());
            }

            std::tuple<core::Pointer, std::string_view, core::JSON> mark{
                entry_pointer, rule->name(), current};
            if (processed_rules.contains(mark)) {
              throw SchemaTransformRuleProcessedTwiceError(rule->name(),
                                                           entry_pointer);
            }

            processed_rules.emplace(std::move(mark));

            if (references_fixed) {
              frame = nullptr;
            }

            if (references_fixed || reframe_after_transform) {
              return true;
            }
          }

          return false;
        })};

    if (!applied) {
      break;
    }
  }

  if (frame == nullptr && !document.is_boolean()) {
    frame = reframe(framer, document);
  }

  if (frame == nullptr) {
    return {true, static_cast<std::uint8_t>(100)};
  }

  return check_rules(document, *frame, this->rules_, walker, resolver, callback,
                     exclude_keyword, true);
}

auto SchemaTransformer::remove(const std::string_view name) -> bool {
  return std::erase_if(this->rules_, [&name](const auto &entry) -> auto {
           return std::get<0>(entry)->name() == name;
         }) > 0;
}

} // namespace sourcemeta::blaze
