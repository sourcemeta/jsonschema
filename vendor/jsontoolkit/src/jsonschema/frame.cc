#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <algorithm>  // std::sort, std::all_of
#include <cassert>    // assert
#include <functional> // std::less
#include <map>        // std::map
#include <optional>   // std::optional
#include <set>        // std::set
#include <sstream>    // std::ostringstream
#include <utility>    // std::pair, std::move
#include <vector>     // std::vector

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

  if (result.empty() ||
      // This means the top-level schema is anonymous
      result.back().second != sourcemeta::jsontoolkit::empty_pointer) {
    result.push_back({"", sourcemeta::jsontoolkit::empty_pointer});
  }

  return result;
}

static auto ref_overrides_adjacent_keywords(const std::string &base_dialect)
    -> bool {
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
         base_dialect == "http://json-schema.org/draft-03/hyper-schema#";
}

static auto supports_id_anchors(const std::string &base_dialect) -> bool {
  return base_dialect == "http://json-schema.org/draft-07/schema#" ||
         base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-06/schema#" ||
         base_dialect == "http://json-schema.org/draft-06/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-04/schema#" ||
         base_dialect == "http://json-schema.org/draft-04/hyper-schema#";
}

static auto fragment_string(const sourcemeta::jsontoolkit::URI &uri)
    -> std::optional<std::string> {
  const auto fragment{uri.fragment()};
  if (fragment.has_value()) {
    return std::string{fragment.value()};
  }

  return std::nullopt;
}

static auto store(sourcemeta::jsontoolkit::Frame::Locations &frame,
                  const sourcemeta::jsontoolkit::ReferenceType type,
                  const sourcemeta::jsontoolkit::Frame::LocationType entry_type,
                  const std::string &uri,
                  const std::optional<std::string> &root_id,
                  const std::string &base_id,
                  const sourcemeta::jsontoolkit::Pointer &pointer_from_root,
                  const sourcemeta::jsontoolkit::Pointer &pointer_from_base,
                  const std::string &dialect, const std::string &base_dialect,
                  const bool ignore_if_present = false) -> void {
  const auto canonical{
      sourcemeta::jsontoolkit::URI{uri}.canonicalize().recompose()};
  const auto inserted{frame
                          .insert({{type, canonical},
                                   {entry_type,
                                    root_id,
                                    base_id,
                                    pointer_from_root,
                                    pointer_from_base,
                                    dialect,
                                    base_dialect,
                                    {}}})
                          .second};
  if (!ignore_if_present && !inserted) {
    std::ostringstream error;
    error << "Schema identifier already exists: " << uri;
    throw sourcemeta::jsontoolkit::SchemaError(error.str());
  }
}

struct InternalEntry {
  const sourcemeta::jsontoolkit::SchemaIteratorEntry common;
  const std::optional<std::string> id;
};

auto internal_analyse(const sourcemeta::jsontoolkit::JSON &schema,
                      sourcemeta::jsontoolkit::Frame::Locations &frame,
                      sourcemeta::jsontoolkit::Frame::References &references,
                      const sourcemeta::jsontoolkit::SchemaWalker &walker,
                      const sourcemeta::jsontoolkit::SchemaResolver &resolver,
                      const std::optional<std::string> &default_dialect,
                      const std::optional<std::string> &default_id) -> void {
  using namespace sourcemeta::jsontoolkit;

  std::vector<InternalEntry> subschema_entries;
  std::set<Pointer> subschemas;
  std::map<sourcemeta::jsontoolkit::Pointer, std::vector<std::string>>
      base_uris;
  std::map<sourcemeta::jsontoolkit::Pointer, std::vector<std::string>>
      base_dialects;

  const std::optional<std::string> root_base_dialect{
      sourcemeta::jsontoolkit::base_dialect(schema, resolver, default_dialect)};
  if (!root_base_dialect.has_value()) {
    throw SchemaError("Cannot determine the base dialect of the schema");
  }

  const std::optional<std::string> root_id{sourcemeta::jsontoolkit::identify(
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
    store(frame, ReferenceType::Static, Frame::LocationType::Resource,
          default_id.value(), root_id.value(), root_id.value(),
          sourcemeta::jsontoolkit::empty_pointer,
          sourcemeta::jsontoolkit::empty_pointer, root_dialect.value(),
          root_base_dialect.value());
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
    std::optional<std::string> id{sourcemeta::jsontoolkit::identify(
        entry.value, entry.base_dialect.value(),
        entry.pointer.empty() ? default_id : std::nullopt)};

    // Store information
    subschema_entries.emplace_back(InternalEntry{entry, std::move(id)});
    subschemas.insert(entry.pointer);
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
          maybe_relative.try_resolve_from(base);
          const std::string new_id{maybe_relative.recompose()};

          if (!maybe_relative_is_absolute ||
              !frame.contains({ReferenceType::Static, new_id})) {
            assert(entry.common.base_dialect.has_value());
            store(frame, ReferenceType::Static, Frame::LocationType::Resource,
                  new_id, root_id, new_id, entry.common.pointer,
                  sourcemeta::jsontoolkit::empty_pointer,
                  entry.common.dialect.value(),
                  entry.common.base_dialect.value());
          }

          if (base_uris.contains(entry.common.pointer)) {
            base_uris.at(entry.common.pointer).push_back(new_id);
          } else {
            base_uris.insert({entry.common.pointer, {new_id}});
          }
        }
      }
    }

    // Handle metaschema references
    const auto maybe_metaschema{
        sourcemeta::jsontoolkit::dialect(entry.common.value)};
    if (maybe_metaschema.has_value()) {
      sourcemeta::jsontoolkit::URI metaschema{maybe_metaschema.value()};
      const auto nearest_bases{
          find_nearest_bases(base_uris, entry.common.pointer, entry.id)};
      if (!nearest_bases.first.empty()) {
        metaschema.try_resolve_from(nearest_bases.first.front());
      }

      metaschema.canonicalize();
      const std::string destination{metaschema.recompose()};
      assert(entry.common.value.defines("$schema"));
      references.insert_or_assign(
          {ReferenceType::Static, entry.common.pointer.concat({"$schema"})},
          Frame::ReferencesEntry{destination,
                                 metaschema.recompose_without_fragment(),
                                 fragment_string(metaschema)});
    }

    // Handle schema anchors
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
          store(frame, ReferenceType::Static, Frame::LocationType::Anchor,
                relative_anchor_uri, root_id, "", entry.common.pointer,
                entry.common.pointer.resolve_from(bases.second),
                entry.common.dialect.value(),
                entry.common.base_dialect.value());
        }

        if (type == sourcemeta::jsontoolkit::AnchorType::Dynamic ||
            type == sourcemeta::jsontoolkit::AnchorType::All) {
          store(frame, ReferenceType::Dynamic, Frame::LocationType::Anchor,
                relative_anchor_uri, root_id, "", entry.common.pointer,
                entry.common.pointer.resolve_from(bases.second),
                entry.common.dialect.value(),
                entry.common.base_dialect.value());

          // Register a dynamic anchor as a static anchor if possible too
          if (entry.common.vocabularies.contains(
                  "https://json-schema.org/draft/2020-12/vocab/core")) {
            store(frame, ReferenceType::Static, Frame::LocationType::Anchor,
                  relative_anchor_uri, root_id, "", entry.common.pointer,
                  entry.common.pointer.resolve_from(bases.second),
                  entry.common.dialect.value(),
                  entry.common.base_dialect.value(), true);
          }
        }
      } else {
        bool is_first = true;
        for (const auto &base_string : bases.first) {
          // TODO: All this dance is necessary because we don't have a
          // URI::fragment setter
          std::ostringstream anchor_uri_string;
          anchor_uri_string << sourcemeta::jsontoolkit::URI{base_string}
                                   .recompose_without_fragment()
                                   .value_or("");
          anchor_uri_string << '#';
          anchor_uri_string << name;
          const auto anchor_uri{
              sourcemeta::jsontoolkit::URI{anchor_uri_string.str()}
                  .canonicalize()
                  .recompose()};

          if (!is_first &&
              frame.contains({ReferenceType::Static, anchor_uri})) {
            continue;
          }

          if (type == sourcemeta::jsontoolkit::AnchorType::Static ||
              type == sourcemeta::jsontoolkit::AnchorType::All) {
            store(frame, sourcemeta::jsontoolkit::ReferenceType::Static,
                  Frame::LocationType::Anchor, anchor_uri, root_id, base_string,
                  entry.common.pointer,
                  entry.common.pointer.resolve_from(bases.second),
                  entry.common.dialect.value(),
                  entry.common.base_dialect.value());
          }

          if (type == sourcemeta::jsontoolkit::AnchorType::Dynamic ||
              type == sourcemeta::jsontoolkit::AnchorType::All) {
            store(frame, sourcemeta::jsontoolkit::ReferenceType::Dynamic,
                  Frame::LocationType::Anchor, anchor_uri, root_id, base_string,
                  entry.common.pointer,
                  entry.common.pointer.resolve_from(bases.second),
                  entry.common.dialect.value(),
                  entry.common.base_dialect.value());

            // Register a dynamic anchor as a static anchor if possible too
            if (entry.common.vocabularies.contains(
                    "https://json-schema.org/draft/2020-12/vocab/core")) {
              store(frame, sourcemeta::jsontoolkit::ReferenceType::Static,
                    Frame::LocationType::Anchor, anchor_uri, root_id,
                    base_string, entry.common.pointer,
                    entry.common.pointer.resolve_from(bases.second),
                    entry.common.dialect.value(),
                    entry.common.base_dialect.value(), true);
            }
          }

          is_first = false;
        }
      }
    }
  }

  // It is important for the loop that follows to assume a specific ordering
  // where smaller pointers (by number of tokens) are scanned first.
  const auto pointer_walker{sourcemeta::jsontoolkit::PointerWalker{schema}};
  std::vector<sourcemeta::jsontoolkit::Pointer> pointers{
      pointer_walker.cbegin(), pointer_walker.cend()};
  std::sort(pointers.begin(), pointers.end(),
            std::less<sourcemeta::jsontoolkit::Pointer>());

  // Pre-compute every possible pointer to the schema
  for (const auto &pointer : pointers) {
    const auto dialects{
        find_nearest_bases(base_dialects, pointer, root_dialect)};
    assert(dialects.first.size() == 1);

    for (const auto &base : find_every_base(base_uris, pointer)) {
      auto relative_pointer_uri{
          sourcemeta::jsontoolkit::to_uri(pointer.resolve_from(base.second))};
      if (!base.first.empty()) {
        relative_pointer_uri.try_resolve_from({base.first});
      }

      relative_pointer_uri.canonicalize();
      const auto result{relative_pointer_uri.recompose()};

      if (!frame.contains({ReferenceType::Static, result})) {
        const auto nearest_bases{
            find_nearest_bases(base_uris, pointer, base.first)};
        assert(!nearest_bases.first.empty());
        const auto &current_base{nearest_bases.first.front()};
        const auto maybe_base_entry{
            frame.find({ReferenceType::Static, current_base})};
        const auto current_base_dialect{
            maybe_base_entry == frame.cend()
                ? root_base_dialect.value()
                : maybe_base_entry->second.base_dialect};
        if (subschemas.contains(pointer)) {
          store(frame, ReferenceType::Static, Frame::LocationType::Subschema,
                result, root_id, current_base, pointer,
                pointer.resolve_from(nearest_bases.second),
                dialects.first.front(), current_base_dialect);
        } else {
          store(frame, ReferenceType::Static, Frame::LocationType::Pointer,
                result, root_id, current_base, pointer,
                pointer.resolve_from(nearest_bases.second),
                dialects.first.front(), current_base_dialect);
        }
      }
    }
  }

  // Resolve references after all framing was performed
  for (const auto &entry : subschema_entries) {
    if (entry.common.value.is_object()) {
      const auto nearest_bases{
          find_nearest_bases(base_uris, entry.common.pointer, entry.id)};
      if (entry.common.value.defines("$ref")) {
        assert(entry.common.value.at("$ref").is_string());
        sourcemeta::jsontoolkit::URI ref{
            entry.common.value.at("$ref").to_string()};
        if (!nearest_bases.first.empty()) {
          ref.try_resolve_from(nearest_bases.first.front());
        }

        ref.canonicalize();
        references.insert_or_assign(
            {ReferenceType::Static, entry.common.pointer.concat({"$ref"})},
            Frame::ReferencesEntry{ref.recompose(),
                                   ref.recompose_without_fragment(),
                                   fragment_string(ref)});
      }

      if (entry.common.vocabularies.contains(
              "https://json-schema.org/draft/2019-09/vocab/core") &&
          entry.common.value.defines("$recursiveRef")) {
        assert(entry.common.value.at("$recursiveRef").is_string());
        const auto &ref{entry.common.value.at("$recursiveRef").to_string()};

        // The behavior of this keyword is defined only for the value "#".
        // Implementations MAY choose to consider other values to be errors.
        // See
        // https://json-schema.org/draft/2019-09/draft-handrews-json-schema-02#rfc.section.8.2.4.2.1
        if (ref != "#") {
          std::ostringstream error;
          error << "Invalid recursive reference: " << ref;
          throw sourcemeta::jsontoolkit::SchemaError(error.str());
        }

        auto anchor_uri_string{
            nearest_bases.first.empty() ? "" : nearest_bases.first.front()};
        const auto recursive_anchor{
            frame.find({ReferenceType::Dynamic, anchor_uri_string})};
        const auto reference_type{recursive_anchor == frame.end()
                                      ? ReferenceType::Static
                                      : ReferenceType::Dynamic};
        const sourcemeta::jsontoolkit::URI anchor_uri{
            std::move(anchor_uri_string)};
        references.insert_or_assign(
            {reference_type, entry.common.pointer.concat({"$recursiveRef"})},
            Frame::ReferencesEntry{anchor_uri.recompose(),
                                   anchor_uri.recompose_without_fragment(),
                                   fragment_string(anchor_uri)});
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
        auto ref_string{ref.recompose()};

        // Note that here we cannot enforce the bookending requirement,
        // as the dynamic reference may point to a schema resource that
        // is not part of or bundled within the schema we are analyzing here.

        const auto has_fragment{ref.fragment().has_value()};
        const auto maybe_static_frame{
            frame.find({ReferenceType::Static, ref_string})};
        const auto maybe_dynamic_frame{
            frame.find({ReferenceType::Dynamic, ref_string})};
        const auto behaves_as_static{!has_fragment ||
                                     (has_fragment &&
                                      maybe_static_frame != frame.end() &&
                                      maybe_dynamic_frame == frame.end())};
        references.insert_or_assign(
            {behaves_as_static ? ReferenceType::Static : ReferenceType::Dynamic,
             entry.common.pointer.concat({"$dynamicRef"})},
            Frame::ReferencesEntry{std::move(ref_string),
                                   ref.recompose_without_fragment(),
                                   fragment_string(ref)});
      }
    }
  }

  // A schema is standalone if all references can be resolved within itself
  const bool standalone{std::all_of(
      references.cbegin(), references.cend(), [&frame](const auto &reference) {
        assert(!reference.first.second.empty());
        assert(reference.first.second.back().is_property());
        // TODO: This check might need to be more elaborate given
        // https://github.com/sourcemeta/jsontoolkit/issues/1390
        return reference.first.second.back().to_property() == "$schema" ||
               frame.contains(
                   {ReferenceType::Static, reference.second.destination}) ||
               frame.contains(
                   {ReferenceType::Dynamic, reference.second.destination});
      })};

  if (standalone) {
    // Find all dynamic anchors
    std::map<std::string, std::vector<std::string>> dynamic_anchors;
    for (const auto &entry : frame) {
      if (entry.first.first != ReferenceType::Dynamic ||
          entry.second.type != Frame::LocationType::Anchor) {
        continue;
      }

      const URI anchor_uri{entry.first.second};
      const std::string fragment{anchor_uri.fragment().value_or("")};
      if (!dynamic_anchors.contains(fragment)) {
        dynamic_anchors.emplace(fragment, std::vector<std::string>{});
      }

      dynamic_anchors[fragment].push_back(entry.first.second);
    }

    // If there is a dynamic reference that only has one possible
    // dynamic anchor destination, then that dynamic reference
    // is a static reference in disguise
    std::vector<Frame::References::key_type> to_delete;
    std::vector<Frame::References::value_type> to_insert;
    for (const auto &reference : references) {
      if (reference.first.first != ReferenceType::Dynamic ||
          !reference.second.fragment.has_value()) {
        continue;
      }

      const auto match{dynamic_anchors.find(reference.second.fragment.value())};
      assert(match != dynamic_anchors.cend());
      // Otherwise we can assume there is only one possible target for the
      // dynamic reference
      if (match->second.size() != 1) {
        continue;
      }

      to_delete.push_back(reference.first);
      const URI new_destination{match->second.front()};
      to_insert.emplace_back(
          Frame::References::key_type{ReferenceType::Static,
                                      reference.first.second},
          Frame::References::mapped_type{
              match->second.front(),
              new_destination.recompose_without_fragment(),
              fragment_string(new_destination)});
    }

    // Because we can't mutate a map as we are traversing it

    for (const auto &key : to_delete) {
      references.erase(key);
    }

    for (auto &&entry : to_insert) {
      references.emplace(std::move(entry));
    }
  }

  for (const auto &reference : references) {
    auto match{
        frame.find({reference.first.first, reference.second.destination})};
    if (match == frame.cend()) {
      continue;
    }

    for (auto &entry : frame) {
      if (entry.second.pointer != match->second.pointer ||
          // Don't count the same origin twice
          std::any_of(entry.second.destination_of.cbegin(),
                      entry.second.destination_of.cend(),
                      [&reference](const auto &destination) {
                        return destination.get() == reference.first;
                      })) {
        continue;
      }

      entry.second.destination_of.emplace_back(reference.first);
    }
  }
}

namespace sourcemeta::jsontoolkit {

auto Frame::analyse(const JSON &schema, const SchemaWalker &walker,
                    const SchemaResolver &resolver,
                    const std::optional<std::string> &default_dialect,
                    const std::optional<std::string> &default_id) -> void {
  internal_analyse(schema, this->locations_, this->references_, walker,
                   resolver, default_dialect, default_id);
}

auto Frame::locations() const noexcept -> const Locations & {
  return this->locations_;
}

auto Frame::references() const noexcept -> const References & {
  return this->references_;
}

auto Frame::vocabularies(const LocationsEntry &location,
                         const SchemaResolver &resolver) const
    -> std::map<std::string, bool> {
  return sourcemeta::jsontoolkit::vocabularies(resolver, location.base_dialect,
                                               location.dialect);
}

auto Frame::uri(const LocationsEntry &location,
                const Pointer &relative_schema_location) const -> std::string {
  return to_uri(location.relative_pointer.concat(relative_schema_location),
                location.base)
      .recompose();
}

auto Frame::traverse(const LocationsEntry &location,
                     const Pointer &relative_schema_location) const
    -> const LocationsEntry & {
  const auto new_uri{this->uri(location, relative_schema_location)};
  const auto static_match{
      this->locations_.find({ReferenceType::Static, new_uri})};
  if (static_match != this->locations_.cend()) {
    return static_match->second;
  }

  const auto dynamic_match{
      this->locations_.find({ReferenceType::Dynamic, new_uri})};
  assert(dynamic_match != this->locations_.cend());
  return dynamic_match->second;
}

auto Frame::traverse(const std::string &uri) const
    -> std::optional<std::reference_wrapper<const LocationsEntry>> {
  const auto static_result{this->locations_.find({ReferenceType::Static, uri})};
  if (static_result != this->locations_.cend()) {
    return static_result->second;
  }

  const auto dynamic_result{
      this->locations_.find({ReferenceType::Dynamic, uri})};
  if (dynamic_result != this->locations_.cend()) {
    return dynamic_result->second;
  }

  return std::nullopt;
}

auto Frame::dereference(const LocationsEntry &location,
                        const Pointer &relative_schema_location) const
    -> std::pair<ReferenceType,
                 std::optional<std::reference_wrapper<const LocationsEntry>>> {
  const auto effective_location{
      location.pointer.concat({relative_schema_location})};
  const auto maybe_reference_entry{
      this->references_.find({ReferenceType::Static, effective_location})};
  if (maybe_reference_entry == this->references_.cend()) {
    // If static dereferencing failed but we know the reference
    // is dynamic, then report so, but without a location, as by
    // definition we can't know the destination until at runtime
    if (this->references_.contains(
            {ReferenceType::Dynamic, effective_location})) {
      return {ReferenceType::Dynamic, std::nullopt};
    }

    return {ReferenceType::Static, std::nullopt};
  }

  const auto destination{this->locations_.find(
      {ReferenceType::Static, maybe_reference_entry->second.destination})};
  assert(destination != this->locations_.cend());
  return {ReferenceType::Static, destination->second};
}

} // namespace sourcemeta::jsontoolkit
