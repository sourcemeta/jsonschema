#include <sourcemeta/core/jsonschema.h>

#include <cassert>   // assert
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

} // namespace sourcemeta::core
