#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <cassert>  // assert
#include <map>      // std::map
#include <optional> // std::optional
#include <sstream>  // std::ostringstream
#include <utility>  // std::pair, std::move
#include <vector>   // std::vector

static auto find_nearest_bases(const std::map<sourcemeta::jsontoolkit::Pointer,
                                              std::vector<std::string>> &bases,
                               const sourcemeta::jsontoolkit::Pointer &pointer,
                               const std::optional<std::string> &default_base)
    -> std::pair<std::vector<std::string>, sourcemeta::jsontoolkit::Pointer> {
  for (const auto &subpointer :
       sourcemeta::jsontoolkit::SubPointerWalker{pointer}) {
    if (bases.contains(subpointer)) {
      return {bases.at(subpointer), subpointer};
    }
  }

  if (default_base.has_value()) {
    return {{default_base.value()}, sourcemeta::jsontoolkit::empty_pointer};
  }

  return {{}, sourcemeta::jsontoolkit::empty_pointer};
}

static auto find_every_base(const std::map<sourcemeta::jsontoolkit::Pointer,
                                           std::vector<std::string>> &bases,
                            const sourcemeta::jsontoolkit::Pointer &pointer)
    -> std::vector<std::pair<std::string, sourcemeta::jsontoolkit::Pointer>> {
  std::vector<std::pair<std::string, sourcemeta::jsontoolkit::Pointer>> result;

  for (const auto &subpointer :
       sourcemeta::jsontoolkit::SubPointerWalker{pointer}) {
    if (bases.contains(subpointer)) {
      for (const auto &base : bases.at(subpointer)) {
        result.push_back({base, subpointer});
      }
    }
  }

  if (result.empty()) {
    result.push_back({"", sourcemeta::jsontoolkit::empty_pointer});
  }

  return result;
}

static auto
ref_overrides_adjacent_keywords(const std::string &base_dialect) -> bool {
  // In older drafts, the presence of `$ref` would override any sibling
  // keywords
  // See
  // https://json-schema.org/draft-07/draft-handrews-json-schema-01#rfc.section.8.3
  return base_dialect == "http://json-schema.org/draft-07/schema#" ||
         base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-06/schema#" ||
         base_dialect == "http://json-schema.org/draft-06/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-04/schema#" ||
         base_dialect == "http://json-schema.org/draft-04/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-03/schema#" ||
         base_dialect == "http://json-schema.org/draft-03/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-02/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-01/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-00/hyper-schema#";
}

static auto supports_id_anchors(const std::string &base_dialect) -> bool {
  return base_dialect == "http://json-schema.org/draft-07/schema#" ||
         base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-06/schema#" ||
         base_dialect == "http://json-schema.org/draft-06/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-04/schema#" ||
         base_dialect == "http://json-schema.org/draft-04/hyper-schema#";
}

static auto fragment_string(const sourcemeta::jsontoolkit::URI uri)
    -> std::optional<std::string> {
  const auto fragment{uri.fragment()};
  if (fragment.has_value()) {
    return std::string{fragment.value()};
  }

  return std::nullopt;
}

static auto store(sourcemeta::jsontoolkit::ReferenceFrame &frame,
                  const sourcemeta::jsontoolkit::ReferenceType type,
                  const std::string &uri,
                  const std::optional<std::string> &root_id,
                  const std::string &base_id,
                  const sourcemeta::jsontoolkit::Pointer &pointer_from_root,
                  const sourcemeta::jsontoolkit::Pointer &pointer_from_base,
                  const std::string &dialect) -> void {
  const auto canonical{
      sourcemeta::jsontoolkit::URI{uri}.canonicalize().recompose()};
  if (!frame
           .insert({{type, canonical},
                    {root_id, base_id, pointer_from_root, pointer_from_base,
                     dialect}})
           .second) {
    std::ostringstream error;
    error << "Schema identifier already exists: " << uri;
    throw sourcemeta::jsontoolkit::SchemaError(error.str());
  }
}

struct InternalEntry {
  const sourcemeta::jsontoolkit::SchemaIteratorEntry common;
  const std::optional<std::string> id;
};

// TODO: Revise this function, try to simplify it, and avoid redundant
// operations (like resolving schemas) by adding relevant overloads
// for the functions it consumes.
auto sourcemeta::jsontoolkit::frame(
    const sourcemeta::jsontoolkit::JSON &schema,
    sourcemeta::jsontoolkit::ReferenceFrame &frame,
    sourcemeta::jsontoolkit::ReferenceMap &references,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect,
    const std::optional<std::string> &default_id) -> std::future<void> {
  std::vector<InternalEntry> subschema_entries;
  std::map<sourcemeta::jsontoolkit::Pointer, std::vector<std::string>>
      base_uris;
  std::map<sourcemeta::jsontoolkit::Pointer, std::vector<std::string>>
      base_dialects;

  const std::optional<std::string> root_base_dialect{
      sourcemeta::jsontoolkit::base_dialect(schema, resolver, default_dialect)
          .get()};
  assert(root_base_dialect.has_value());

  const std::optional<std::string> root_id{sourcemeta::jsontoolkit::id(
      schema, root_base_dialect.value(), default_id)};
  const std::optional<std::string> root_dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  assert(root_dialect.has_value());

  // If the top-level schema has a specific identifier but the user
  // passes a different default identifier, then the schema is by
  // definition known by two names, and we should handle that accordingly
  const bool has_explicit_different_id{root_id.has_value() &&
                                       default_id.has_value() &&
                                       root_id.value() != default_id.value()};
  if (has_explicit_different_id) {
    store(frame, ReferenceType::Static, default_id.value(), root_id.value(),
          root_id.value(), sourcemeta::jsontoolkit::empty_pointer,
          sourcemeta::jsontoolkit::empty_pointer, root_dialect.value());
    base_uris.insert(
        {sourcemeta::jsontoolkit::empty_pointer, {default_id.value()}});
  }

  for (auto &entry : sourcemeta::jsontoolkit::SchemaIterator{
           schema, walker, resolver, default_dialect}) {
    // Dialect
    assert(entry.dialect.has_value());
    base_dialects.insert({entry.pointer, {entry.dialect.value()}});

    // Base dialect
    assert(entry.base_dialect.has_value());

    // Schema identifier
    std::optional<std::string> id{sourcemeta::jsontoolkit::id(
        entry.value, entry.base_dialect.value(),
        entry.pointer.empty() ? default_id : std::nullopt)};

    // Store information
    subschema_entries.emplace_back(
        InternalEntry{std::move(entry), std::move(id)});
  }

  for (const auto &entry : subschema_entries) {
    if (entry.id.has_value()) {
      const bool ref_overrides =
          ref_overrides_adjacent_keywords(entry.common.base_dialect.value());
      const bool is_pre_2019_09_location_independent_identifier =
          supports_id_anchors(entry.common.base_dialect.value()) &&
          sourcemeta::jsontoolkit::URI{entry.id.value()}.is_fragment_only();

      if ((!entry.common.value.defines("$ref") || !ref_overrides) &&
          // If we are dealing with a pre-2019-09 location independent
          // identifier, we ignore it as a traditional identifier and take care
          // of it as an anchor
          !is_pre_2019_09_location_independent_identifier) {
        const auto bases{
            find_nearest_bases(base_uris, entry.common.pointer, entry.id)};
        for (const auto &base_string : bases.first) {
          const sourcemeta::jsontoolkit::URI base{base_string};
          sourcemeta::jsontoolkit::URI maybe_relative{entry.id.value()};
          const auto maybe_fragment{maybe_relative.fragment()};

          // See
          // https://json-schema.org/draft/2019-09/draft-handrews-json-schema-02#rfc.section.8.2.2
          // See
          // https://json-schema.org/draft/2020-12/draft-bhutton-json-schema-01#section-8.2.1-5
          if (maybe_fragment.has_value() && !maybe_fragment.value().empty()) {
            throw SchemaError(
                "Identifiers must not contain non-empty fragments");
          }

          const bool maybe_relative_is_absolute{maybe_relative.is_absolute()};
          maybe_relative.resolve_from(base);
          const std::string new_id{maybe_relative.recompose()};

          if (!maybe_relative_is_absolute ||
              !frame.contains({ReferenceType::Static, new_id})) {
            store(frame, ReferenceType::Static, new_id, root_id, new_id,
                  entry.common.pointer, sourcemeta::jsontoolkit::empty_pointer,
                  entry.common.dialect.value());
          }

          if (base_uris.contains(entry.common.pointer)) {
            base_uris.at(entry.common.pointer).push_back(new_id);
          } else {
            base_uris.insert({entry.common.pointer, {new_id}});
          }
        }
      }
    }

    // Handle schema anchors
    // TODO: Support $recursiveAnchor
    for (const auto &[name, type] : sourcemeta::jsontoolkit::anchors(
             entry.common.value, entry.common.vocabularies)) {
      const auto bases{
          find_nearest_bases(base_uris, entry.common.pointer, entry.id)};

      if (bases.first.empty()) {
        const auto anchor_uri{
            sourcemeta::jsontoolkit::URI::from_fragment(name)};
        const auto relative_anchor_uri{anchor_uri.recompose()};

        if (type == sourcemeta::jsontoolkit::AnchorType::Static ||
            type == sourcemeta::jsontoolkit::AnchorType::All) {
          store(frame, ReferenceType::Static, relative_anchor_uri, root_id, "",
                entry.common.pointer,
                entry.common.pointer.resolve_from(bases.second),
                entry.common.dialect.value());
        }

        if (type == sourcemeta::jsontoolkit::AnchorType::Dynamic ||
            type == sourcemeta::jsontoolkit::AnchorType::All) {
          store(frame, ReferenceType::Dynamic, relative_anchor_uri, root_id, "",
                entry.common.pointer,
                entry.common.pointer.resolve_from(bases.second),
                entry.common.dialect.value());
        }
      } else {
        bool is_first = true;
        for (const auto &base_string : bases.first) {
          auto anchor_uri{sourcemeta::jsontoolkit::URI::from_fragment(name)};
          const sourcemeta::jsontoolkit::URI anchor_base{base_string};
          anchor_uri.resolve_from_if_absolute(anchor_base);
          const auto absolute_anchor_uri{anchor_uri.recompose()};

          if (!is_first &&
              frame.contains({ReferenceType::Static, absolute_anchor_uri})) {
            continue;
          }

          if (type == sourcemeta::jsontoolkit::AnchorType::Static ||
              type == sourcemeta::jsontoolkit::AnchorType::All) {
            store(frame, sourcemeta::jsontoolkit::ReferenceType::Static,
                  absolute_anchor_uri, root_id, base_string,
                  entry.common.pointer,
                  entry.common.pointer.resolve_from(bases.second),
                  entry.common.dialect.value());
          }

          if (type == sourcemeta::jsontoolkit::AnchorType::Dynamic ||
              type == sourcemeta::jsontoolkit::AnchorType::All) {
            store(frame, sourcemeta::jsontoolkit::ReferenceType::Dynamic,
                  absolute_anchor_uri, root_id, base_string,
                  entry.common.pointer,
                  entry.common.pointer.resolve_from(bases.second),
                  entry.common.dialect.value());
          }

          is_first = false;
        }
      }
    }
  }

  // Pre-compute every possible pointer to the schema
  for (const auto &pointer : sourcemeta::jsontoolkit::PointerWalker{schema}) {
    const auto dialects{
        find_nearest_bases(base_dialects, pointer, root_dialect)};
    assert(dialects.first.size() == 1);

    for (const auto &base : find_every_base(base_uris, pointer)) {
      auto relative_pointer_uri{
          sourcemeta::jsontoolkit::to_uri(pointer.resolve_from(base.second))};
      if (!base.first.empty()) {
        relative_pointer_uri.resolve_from({base.first});
      }

      relative_pointer_uri.canonicalize();
      const auto result{relative_pointer_uri.recompose()};

      if (!frame.contains({ReferenceType::Static, result})) {
        const auto nearest_bases{
            find_nearest_bases(base_uris, pointer, base.first)};
        assert(!nearest_bases.first.empty());
        store(frame, ReferenceType::Static, result, root_id,
              nearest_bases.first.front(), pointer,
              pointer.resolve_from(nearest_bases.second),
              dialects.first.front());
      }
    }
  }

  // Resolve references after all framing was performed
  for (const auto &entry : subschema_entries) {
    // TODO: Handle $recursiveRef too
    if (entry.common.value.is_object()) {
      const auto nearest_bases{
          find_nearest_bases(base_uris, entry.common.pointer, entry.id)};

      // TODO: Check that static destinations actually exist in the frame
      if (entry.common.value.defines("$ref")) {
        assert(entry.common.value.at("$ref").is_string());
        sourcemeta::jsontoolkit::URI ref{
            entry.common.value.at("$ref").to_string()};
        if (!nearest_bases.first.empty()) {
          ref.resolve_from(nearest_bases.first.front());
        }

        ref.canonicalize();
        references.insert(
            {{ReferenceType::Static, entry.common.pointer.concat({"$ref"})},
             {ref.recompose(), ref.recompose_without_fragment(),
              fragment_string(ref)}});
      }

      if (entry.common.vocabularies.contains(
              "https://json-schema.org/draft/2020-12/vocab/core") &&
          entry.common.value.defines("$dynamicRef")) {
        assert(entry.common.value.at("$dynamicRef").is_string());
        sourcemeta::jsontoolkit::URI ref{
            entry.common.value.at("$dynamicRef").to_string()};
        if (!nearest_bases.first.empty()) {
          ref.resolve_from(nearest_bases.first.front());
        }

        ref.canonicalize();
        // TODO: Check bookending requirement
        const auto destination{ref.recompose()};
        // TODO: We shouldn't need to reparse if the URI handled mutations
        const sourcemeta::jsontoolkit::URI destination_uri{destination};
        references.insert(
            {{ReferenceType::Dynamic,
              entry.common.pointer.concat({"$dynamicRef"})},
             {destination, destination_uri.recompose_without_fragment(),
              fragment_string(destination_uri)}});
      }
    }
  }

  return std::promise<void>{}.get_future();
}
