#include <sourcemeta/core/jsonschema.h>

#include <cassert>   // assert
#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error
#include <utility>   // std::move, std::pair

namespace {

auto is_true(const sourcemeta::core::SchemaTransformRule::Result &result)
    -> bool {
  switch (result.index()) {
    case 0:
      assert(std::holds_alternative<bool>(result));
      return *std::get_if<bool>(&result);
    default:
      assert(std::holds_alternative<std::string>(result));
      return true;
  }
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

auto SchemaTransformRule::transform(JSON &) const -> void {
  throw SchemaAbortError("This rule cannot be automatically transformed");
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
  if (!is_true(outcome)) {
    return {true, std::move(outcome)};
  }

  try {
    this->transform(schema);
  } catch (const SchemaAbortError &) {
    return {false, std::move(outcome)};
  }

  // The condition must always be false after applying the
  // transformation in order to avoid infinite loops
  if (is_true(this->condition(schema, root, vocabularies, frame, location,
                              walker, resolver))) {
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
    const std::optional<std::string> &default_dialect) const -> bool {
  SchemaFrame frame{SchemaFrame::Mode::Locations};
  frame.analyse(schema, walker, resolver, default_dialect);

  bool result{true};
  for (const auto &entry : frame.locations()) {
    if (entry.second.type != SchemaFrame::LocationType::Resource &&
        entry.second.type != SchemaFrame::LocationType::Subschema) {
      continue;
    }

    const auto &current{get(schema, entry.second.pointer)};
    const auto current_vocabularies{
        vocabularies(schema, resolver, entry.second.dialect)};
    for (const auto &[name, rule] : this->rules) {
      const auto outcome{rule->check(current, schema, current_vocabularies,
                                     walker, resolver, frame, entry.second)};
      switch (outcome.index()) {
        case 0:
          assert(std::holds_alternative<bool>(outcome));
          if (*std::get_if<bool>(&outcome)) {
            result = false;
            callback(entry.second.pointer, name, rule->message(), "");
          }

          break;
        default:
          assert(std::holds_alternative<std::string>(outcome));
          result = false;
          callback(entry.second.pointer, name, rule->message(),
                   *std::get_if<std::string>(&outcome));
          break;
      }
    }
  }

  return result;
}

auto SchemaTransformer::apply(
    JSON &schema, const SchemaWalker &walker, const SchemaResolver &resolver,
    const SchemaTransformer::Callback &callback,
    const std::optional<std::string> &default_dialect) const -> bool {
  // There is no point in applying an empty bundle
  assert(!this->rules.empty());
  std::set<std::pair<Pointer, JSON::String>> processed_rules;

  bool result{true};
  while (true) {
    SchemaFrame frame{SchemaFrame::Mode::Locations};
    frame.analyse(schema, walker, resolver, default_dialect);

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
          applied = is_true(subresult.second) || applied;
        } else {
          result = false;
          callback(entry.second.pointer, name, rule->message(),
                   subresult.second.index() == 0
                       ? ""
                       : *std::get_if<std::string>(&subresult.second));
        }

        if (!applied) {
          continue;
        }

        if (processed_rules.contains({entry.second.pointer, name})) {
          // TODO: Throw a better custom error that also highlights the schema
          // location
          std::ostringstream error;
          error << "Rules must only be processed once: " << name;
          throw std::runtime_error(error.str());
        }

        processed_rules.emplace(entry.second.pointer, name);
        goto alterschema_start_again;
      }
    }

  alterschema_start_again:
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
