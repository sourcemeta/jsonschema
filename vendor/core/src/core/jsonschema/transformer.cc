#include <sourcemeta/core/jsonschema.h>

#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

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
