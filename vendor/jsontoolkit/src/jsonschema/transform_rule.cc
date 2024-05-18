#include <sourcemeta/jsontoolkit/jsonschema.h>

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

sourcemeta::jsontoolkit::SchemaTransformRule::SchemaTransformRule(
    std::string &&name, std::string &&message)
    : name_{std::move(name)}, message_{std::move(message)} {}

auto sourcemeta::jsontoolkit::SchemaTransformRule::operator==(
    const sourcemeta::jsontoolkit::SchemaTransformRule &other) const -> bool {
  return this->name() == other.name();
}

auto sourcemeta::jsontoolkit::SchemaTransformRule::name() const
    -> const std::string & {
  return this->name_;
}

auto sourcemeta::jsontoolkit::SchemaTransformRule::message() const
    -> const std::string & {
  return this->message_;
}

auto sourcemeta::jsontoolkit::SchemaTransformRule::apply(
    sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::Pointer &pointer,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) const
    -> std::vector<SchemaTransformerOperation> {
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  if (!dialect.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaError(
        "Could not determine the schema dialect");
  }

  const auto vocabularies{vocabularies_to_set(
      sourcemeta::jsontoolkit::vocabularies(schema, resolver, default_dialect)
          .get())};
  if (!this->condition(schema, dialect.value(), vocabularies, pointer)) {
    return {};
  }

  sourcemeta::jsontoolkit::SchemaTransformer transformer{schema};
  this->transform(transformer);
  // Otherwise the transformation didn't do anything
  assert(!transformer.traces().empty());

  // The condition must always be false after applying the
  // transformation in order to avoid infinite loops
  if (this->condition(schema, dialect.value(), vocabularies, pointer)) {
    std::ostringstream error;
    error << "Rule condition holds after application: " << this->name();
    throw std::runtime_error(error.str());
  }

  return transformer.traces();
}

auto sourcemeta::jsontoolkit::SchemaTransformRule::check(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::Pointer &pointer,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) const -> bool {
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  if (!dialect.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaError(
        "Could not determine the schema dialect");
  }

  const auto vocabularies{vocabularies_to_set(
      sourcemeta::jsontoolkit::vocabularies(schema, resolver, default_dialect)
          .get())};
  return this->condition(schema, dialect.value(), vocabularies, pointer);
}
