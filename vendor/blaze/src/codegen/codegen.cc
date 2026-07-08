#include <sourcemeta/blaze/bundle.h>
#include <sourcemeta/blaze/canonicalizer.h>
#include <sourcemeta/blaze/codegen.h>
#include <sourcemeta/blaze/frame.h>

#include <algorithm>     // std::ranges::sort
#include <cassert>       // assert
#include <unordered_set> // std::unordered_set

#include "codegen_default_compiler.h"

namespace {

auto is_validation_subschema(
    const sourcemeta::blaze::SchemaFrame &frame,
    const sourcemeta::blaze::SchemaFrame::Location &location,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::SchemaResolver &resolver) -> bool {
  if (!location.parent.has_value()) {
    return false;
  }

  const auto &parent{location.parent.value()};
  if (parent.size() >= location.pointer.size()) {
    return false;
  }

  const auto &keyword_token{location.pointer.at(parent.size())};
  if (!keyword_token.is_property()) {
    return false;
  }

  const auto parent_location{frame.traverse(parent)};
  if (!parent_location.has_value()) {
    return false;
  }

  const auto vocabularies{
      frame.vocabularies(parent_location.value().get(), resolver)};
  const auto &walker_result{walker(keyword_token.to_property(), vocabularies)};
  using Type = sourcemeta::blaze::SchemaKeywordType;
  if (walker_result.type == Type::ApplicatorValueTraverseAnyPropertyKey ||
      walker_result.type == Type::ApplicatorValueTraverseAnyItem) {
    return true;
  }

  return is_validation_subschema(frame, parent_location.value().get(), walker,
                                 resolver);
}

} // anonymous namespace

namespace sourcemeta::blaze {

auto compile(const sourcemeta::core::JSON &input,
             const sourcemeta::blaze::SchemaWalker &walker,
             const sourcemeta::blaze::SchemaResolver &resolver,
             const CodegenCompiler &compiler,
             const std::string_view default_dialect,
             const std::string_view default_id) -> CodegenIRResult {
  // --------------------------------------------------------------------------
  // (1) Bundle the schema to resolve external references
  // --------------------------------------------------------------------------

  auto schema{sourcemeta::blaze::bundle(
      input, walker, resolver, sourcemeta::blaze::BundleMode::References,
      default_dialect, default_id)};

  // --------------------------------------------------------------------------
  // (2) Canonicalize the schema for easier analysis
  // --------------------------------------------------------------------------

  sourcemeta::blaze::canonicalize(schema, walker, resolver, default_dialect,
                                  default_id);

  // --------------------------------------------------------------------------
  // (3) Frame the resulting schema with instance location information
  // --------------------------------------------------------------------------

  sourcemeta::blaze::SchemaFrame frame{
      sourcemeta::blaze::SchemaFrame::Mode::References};
  frame.analyse(schema, walker, resolver, default_dialect, default_id);

  // --------------------------------------------------------------------------
  // (4) Convert every subschema into a code generation object
  // --------------------------------------------------------------------------

  std::unordered_set<sourcemeta::core::WeakPointer,
                     sourcemeta::core::WeakPointer::Hasher,
                     sourcemeta::core::WeakPointer::Comparator>
      visited;
  CodegenIRResult result;
  for (const auto &[key, location] : frame.locations()) {
    if (location.type !=
            sourcemeta::blaze::SchemaFrame::LocationType::Resource &&
        location.type !=
            sourcemeta::blaze::SchemaFrame::LocationType::Subschema) {
      continue;
    }

    // Framing may report resource twice or more given default identifiers and
    // nested resources
    const auto [visited_iterator, inserted] = visited.insert(location.pointer);
    if (!inserted) {
      continue;
    }

    // Skip subschemas under validation-only keywords that do not contribute
    // to the type structure (like `contains`)
    if (is_validation_subschema(frame, location, walker, resolver)) {
      continue;
    }

    const auto &subschema{sourcemeta::core::get(schema, location.pointer)};
    result.push_back(compiler(schema, frame, location, resolver, subschema));
  }

  // --------------------------------------------------------------------------
  // (5) Sort entries so that dependencies come before dependents
  // --------------------------------------------------------------------------

  std::ranges::sort(
      result,
      [](const CodegenIREntity &left, const CodegenIREntity &right) -> bool {
        return std::visit(
                   [](const auto &entry) -> auto { return entry.pointer; },
                   right) <
               std::visit(
                   [](const auto &entry) -> auto { return entry.pointer; },
                   left);
      });

  return result;
}

} // namespace sourcemeta::blaze
