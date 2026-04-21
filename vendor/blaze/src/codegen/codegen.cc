#include <sourcemeta/blaze/alterschema.h>
#include <sourcemeta/blaze/codegen.h>

#include <algorithm>     // std::ranges::sort
#include <cassert>       // assert
#include <unordered_set> // std::unordered_set

#include "codegen_default_compiler.h"

namespace {

auto is_validation_subschema(
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaFrame::Location &location,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver) -> bool {
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
  using Type = sourcemeta::core::SchemaKeywordType;
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
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const CodegenCompiler &compiler,
             const std::string_view default_dialect,
             const std::string_view default_id) -> CodegenIRResult {
  // --------------------------------------------------------------------------
  // (1) Bundle the schema to resolve external references
  // --------------------------------------------------------------------------

  auto schema{sourcemeta::core::bundle(input, walker, resolver, default_dialect,
                                       default_id)};

  // --------------------------------------------------------------------------
  // (2) Canonicalize the schema for easier analysis
  // --------------------------------------------------------------------------

  sourcemeta::blaze::SchemaTransformer canonicalizer;
  sourcemeta::blaze::add(canonicalizer,
                         sourcemeta::blaze::AlterSchemaMode::Canonicalizer);
  [[maybe_unused]] const auto canonicalized{canonicalizer.apply(
      schema, walker, resolver,
      [](const auto &, const auto, const auto, const auto &,
         [[maybe_unused]] const auto applied) { assert(applied); },
      default_dialect, default_id)};
  assert(canonicalized.first);

  // --------------------------------------------------------------------------
  // (3) Frame the resulting schema with instance location information
  // --------------------------------------------------------------------------

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
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
            sourcemeta::core::SchemaFrame::LocationType::Resource &&
        location.type !=
            sourcemeta::core::SchemaFrame::LocationType::Subschema) {
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
        return std::visit([](const auto &entry) { return entry.pointer; },
                          right) <
               std::visit([](const auto &entry) { return entry.pointer; },
                          left);
      });

  return result;
}

} // namespace sourcemeta::blaze
