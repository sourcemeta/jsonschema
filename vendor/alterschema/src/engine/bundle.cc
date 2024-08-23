#include <sourcemeta/alterschema/engine.h>

#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error

auto sourcemeta::alterschema::Bundle::apply(
    sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const sourcemeta::jsontoolkit::Pointer &pointer,
    const std::optional<std::string> &default_dialect) const -> void {
  // There is no point in applying an empty bundle
  assert(!this->rules.empty());

  auto &current{sourcemeta::jsontoolkit::get(schema, pointer)};
  const std::optional<std::string> root_dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(current, root_dialect)};

  // (1) Transform the current schema object
  // Avoid recursion to not blow up the stack even on highly complex schemas
  std::set<std::string> processed_rules;
  while (true) {
    auto matches{processed_rules.size()};
    for (const auto &[name, rule] : this->rules) {
      // TODO: Process traces to fixup references
      const auto traces{rule->apply(current, pointer, resolver, dialect)};
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
  for (const auto &entry : sourcemeta::jsontoolkit::SchemaIteratorFlat{
           current, walker, resolver, dialect}) {
    apply(schema, walker, resolver, pointer.concat(entry.pointer), dialect);
  }
}

auto sourcemeta::alterschema::Bundle::check(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const sourcemeta::alterschema::Bundle::CheckCallback &callback,
    const sourcemeta::jsontoolkit::Pointer &pointer,
    const std::optional<std::string> &default_dialect) const -> bool {
  const auto &current{sourcemeta::jsontoolkit::get(schema, pointer)};
  const std::optional<std::string> root_dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(current, root_dialect)};

  bool result{true};
  for (const auto &entry : sourcemeta::jsontoolkit::SchemaIterator{
           current, walker, resolver, dialect}) {
    const auto current_pointer{pointer.concat(entry.pointer)};
    for (const auto &[name, rule] : this->rules) {
      if (rule->check(sourcemeta::jsontoolkit::get(current, entry.pointer),
                      current_pointer, resolver, dialect)) {
        result = false;
        callback(current_pointer, name, rule->message());
      }
    }
  }

  return result;
}
