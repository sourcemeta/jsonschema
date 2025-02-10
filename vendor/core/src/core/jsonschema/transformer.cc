#include <sourcemeta/core/jsonschema.h>

#include <cassert>   // assert
#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error
#include <utility>   // std::move

namespace {

auto vocabularies_to_set(const std::map<std::string, bool> &vocabularies)
    -> std::set<std::string> {
  std::set<std::string> result;
  for (const auto &pair : vocabularies) {
    result.insert(pair.first);
  }

  return result;
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

auto SchemaTransformRule::apply(
    JSON &schema, const Pointer &pointer, const SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) const
    -> std::vector<PointerProxy::Operation> {
  const std::optional<std::string> effective_dialect{
      dialect(schema, default_dialect)};
  if (!effective_dialect.has_value()) {
    throw SchemaError("Could not determine the schema dialect");
  }

  const auto current_vocabularies{
      vocabularies_to_set(vocabularies(schema, resolver, default_dialect))};
  if (!this->condition(schema, effective_dialect.value(), current_vocabularies,
                       pointer)) {
    return {};
  }

  PointerProxy transformer{schema};
  this->transform(transformer);
  // Otherwise the transformation didn't do anything
  assert(!transformer.traces().empty());

  // The condition must always be false after applying the
  // transformation in order to avoid infinite loops
  if (this->condition(schema, effective_dialect.value(), current_vocabularies,
                      pointer)) {
    std::ostringstream error;
    error << "Rule condition holds after application: " << this->name();
    throw std::runtime_error(error.str());
  }

  return transformer.traces();
}

auto SchemaTransformRule::check(
    const JSON &schema, const Pointer &pointer, const SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) const -> bool {
  const std::optional<std::string> effective_dialect{
      dialect(schema, default_dialect)};
  if (!effective_dialect.has_value()) {
    throw SchemaError("Could not determine the schema dialect");
  }

  return this->condition(
      schema, effective_dialect.value(),
      vocabularies_to_set(vocabularies(schema, resolver, default_dialect)),
      pointer);
}

auto SchemaTransformer::apply(
    JSON &schema, const SchemaWalker &walker, const SchemaResolver &resolver,
    const Pointer &pointer,
    const std::optional<std::string> &default_dialect) const -> void {
  // There is no point in applying an empty bundle
  assert(!this->rules.empty());

  auto &current{get(schema, pointer)};
  const std::optional<std::string> root_dialect{
      dialect(schema, default_dialect)};
  const std::optional<std::string> effective_dialect{
      dialect(current, root_dialect)};

  // (1) Transform the current schema object
  // Avoid recursion to not blow up the stack even on highly complex schemas
  std::set<std::string> processed_rules;
  while (true) {
    auto matches{processed_rules.size()};
    for (const auto &[name, rule] : this->rules) {
      // TODO: Process traces to fixup references
      const auto traces{
          rule->apply(current, pointer, resolver, effective_dialect)};
      if (!traces.empty()) {
        if (processed_rules.contains(name)) {
          std::ostringstream error;
          error << "Rules must only be processed once: " << name;
          throw std::runtime_error(error.str());
        }

        processed_rules.insert(name);
      }
    }

    if (matches < processed_rules.size()) {
      continue;
    }

    break;
  }

  // (2) Transform its sub-schemas
  for (const auto &entry :
       // TODO: Replace `SchemaIteratorFlat` with framing and then just get
       // rid of the idea of flat iterators, as we don't need it anywhere else
       SchemaIteratorFlat{current, walker, resolver, effective_dialect}) {
    apply(schema, walker, resolver, pointer.concat(entry.pointer),
          effective_dialect);
  }
}

auto SchemaTransformer::check(
    const JSON &schema, const SchemaWalker &walker,
    const SchemaResolver &resolver,
    const SchemaTransformer::CheckCallback &callback, const Pointer &pointer,
    const std::optional<std::string> &default_dialect) const -> bool {
  const auto &current{get(schema, pointer)};
  const std::optional<std::string> root_dialect{
      dialect(schema, default_dialect)};
  const std::optional<std::string> effective_dialect{
      dialect(current, root_dialect)};

  bool result{true};
  for (const auto &entry :
       SchemaIterator{current, walker, resolver, effective_dialect}) {
    const auto current_pointer{pointer.concat(entry.pointer)};
    for (const auto &[name, rule] : this->rules) {
      if (rule->check(get(current, entry.pointer), current_pointer, resolver,
                      effective_dialect)) {
        result = false;
        callback(current_pointer, name, rule->message());
      }
    }
  }

  return result;
}

} // namespace sourcemeta::core
