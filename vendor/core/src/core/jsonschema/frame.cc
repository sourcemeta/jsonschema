#include <sourcemeta/core/jsonschema.h>

#include <algorithm>     // std::sort, std::all_of, std::any_of
#include <cassert>       // assert
#include <functional>    // std::less
#include <map>           // std::map
#include <optional>      // std::optional
#include <sstream>       // std::ostringstream
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::pair, std::move
#include <vector>        // std::vector

enum class AnchorType : std::uint8_t { Static, Dynamic, All };

namespace {

auto find_anchors(const sourcemeta::core::JSON &schema,
                  const sourcemeta::core::Vocabularies &vocabularies)
    -> std::map<sourcemeta::core::JSON::String, AnchorType> {
  std::map<sourcemeta::core::JSON::String, AnchorType> result;

  // 2020-12
  if (schema.is_object() &&
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_2020_12_Core)) {
    if (schema.defines("$dynamicAnchor")) {
      const auto &anchor{schema.at("$dynamicAnchor")};
      if (anchor.is_string()) {
        result.insert({anchor.to_string(), AnchorType::Dynamic});
      }
    }

    if (schema.defines("$anchor")) {
      const auto &anchor{schema.at("$anchor")};
      if (anchor.is_string()) {
        const auto anchor_string{anchor.to_string()};
        const auto success = result.insert({anchor_string, AnchorType::Static});
        assert(success.second || result.contains(anchor_string));
        if (!success.second) {
          result[anchor_string] = AnchorType::All;
        }
      }
    }
  }

  // 2019-09
  if (schema.is_object() &&
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_2019_09_Core)) {
    if (schema.defines("$recursiveAnchor")) {
      const auto &anchor{schema.at("$recursiveAnchor")};
      assert(anchor.is_boolean());
      if (anchor.to_boolean()) {
        // We store a 2019-09 recursive anchor as an empty anchor
        result.insert({"", AnchorType::Dynamic});
      }
    }

    if (schema.defines("$anchor")) {
      const auto &anchor{schema.at("$anchor")};
      if (anchor.is_string()) {
        const auto anchor_string{anchor.to_string()};
        const auto success = result.insert({anchor_string, AnchorType::Static});
        assert(success.second || result.contains(anchor_string));
        if (!success.second) {
          result[anchor_string] = AnchorType::All;
        }
      }
    }
  }

  // Draft 7 and 6
  // Old `$id` anchor form
  if (schema.is_object() &&
      (vocabularies.contains(
           sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_7) ||
       vocabularies.contains(
           sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_6))) {
    if (schema.defines("$id")) {
      assert(schema.at("$id").is_string());
      const sourcemeta::core::URI identifier(schema.at("$id").to_string());
      if (identifier.is_fragment_only()) {
        result.insert(
            {sourcemeta::core::JSON::String{
                 // Check for optional is happening inside is_fragment_only()
                 // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                 identifier.fragment().value()},
             AnchorType::Static});
      }
    }
  }

  // Draft 4
  // Old `id` anchor form
  if (schema.is_object() &&
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_4)) {
    if (schema.defines("id")) {
      assert(schema.at("id").is_string());
      const sourcemeta::core::URI identifier(schema.at("id").to_string());
      if (identifier.is_fragment_only()) {
        result.insert(
            {sourcemeta::core::JSON::String{
                 // Check for optional is happening inside is_fragment_only()
                 // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                 identifier.fragment().value()},
             AnchorType::Static});
      }
    }
  }

  return result;
}

auto find_nearest_bases(
    const std::unordered_map<sourcemeta::core::Pointer,
                             std::vector<sourcemeta::core::JSON::String>>
        &bases,
    const sourcemeta::core::Pointer &pointer,
    const std::optional<sourcemeta::core::JSON::String> &default_base)
    -> std::pair<std::vector<sourcemeta::core::JSON::String>,
                 sourcemeta::core::Pointer> {
  auto current_pointer{pointer};
  while (true) {
    const auto match{bases.find(current_pointer)};
    if (match != bases.cend()) {
      return {match->second, current_pointer};
    }

    if (current_pointer.empty()) {
      break;
    }

    current_pointer = current_pointer.initial();
  }

  if (default_base.has_value()) {
    return {{default_base.value()}, sourcemeta::core::empty_pointer};
  }

  return {{}, sourcemeta::core::empty_pointer};
}

auto find_every_base(
    const std::unordered_map<sourcemeta::core::Pointer,
                             std::vector<sourcemeta::core::JSON::String>>
        &bases,
    const sourcemeta::core::Pointer &pointer)
    -> std::vector<
        std::pair<sourcemeta::core::JSON::String, sourcemeta::core::Pointer>> {
  std::vector<
      std::pair<sourcemeta::core::JSON::String, sourcemeta::core::Pointer>>
      result;

  auto current_pointer{pointer};
  while (true) {
    const auto match{bases.find(current_pointer)};
    if (match != bases.cend()) {
      for (const auto &base : match->second) {
        result.emplace_back(base, current_pointer);
      }
    }

    if (current_pointer.empty()) {
      break;
    }

    current_pointer = current_pointer.initial();
  }

  if (result.empty() ||
      result.back().second != sourcemeta::core::empty_pointer) {
    result.emplace_back("", sourcemeta::core::empty_pointer);
  }

  return result;
}

// TODO: Why do we have this function both here and on `walker.cc`?
auto ref_overrides_adjacent_keywords(std::string_view base_dialect) -> bool {
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

auto supports_id_anchors(std::string_view base_dialect) -> bool {
  return base_dialect == "http://json-schema.org/draft-07/schema#" ||
         base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-06/schema#" ||
         base_dialect == "http://json-schema.org/draft-06/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-04/schema#" ||
         base_dialect == "http://json-schema.org/draft-04/hyper-schema#";
}

auto fragment_string(const sourcemeta::core::URI &uri)
    -> std::optional<sourcemeta::core::JSON::String> {
  const auto fragment{uri.fragment()};
  if (fragment.has_value()) {
    return sourcemeta::core::JSON::String{fragment.value()};
  }

  return std::nullopt;
}

[[noreturn]]
auto throw_already_exists(const sourcemeta::core::JSON::String &uri) -> void {
  throw sourcemeta::core::SchemaFrameError(uri,
                                           "Schema identifier already exists");
}

auto store(sourcemeta::core::SchemaFrame::Locations &frame,
           const sourcemeta::core::SchemaReferenceType type,
           const sourcemeta::core::SchemaFrame::LocationType entry_type,
           const sourcemeta::core::JSON::String &uri,
           const std::optional<sourcemeta::core::JSON::String> &root_id,
           const sourcemeta::core::JSON::String &base_id,
           const sourcemeta::core::Pointer &pointer_from_root,
           const sourcemeta::core::Pointer &pointer_from_base,
           const sourcemeta::core::JSON::String &dialect,
           const sourcemeta::core::JSON::String &base_dialect,
           const std::optional<sourcemeta::core::Pointer> &parent,
           const bool ignore_if_present = false,
           const bool already_canonical = false) -> void {
  const auto canonical{
      already_canonical ? uri : sourcemeta::core::URI::canonicalize(uri)};
  const auto inserted{frame
                          .insert({{type, canonical},
                                   {.parent = parent,
                                    .type = entry_type,
                                    .root = root_id,
                                    .base = base_id,
                                    .pointer = pointer_from_root,
                                    .relative_pointer = pointer_from_base,
                                    .dialect = dialect,
                                    .base_dialect = base_dialect}})
                          .second};
  if (!ignore_if_present && !inserted) {
    throw_already_exists(canonical);
  }
}

// Check misunderstood struct to be a function
// NOLINTNEXTLINE(bugprone-exception-escape)
struct InternalEntry {
  sourcemeta::core::SchemaIteratorEntry common;
  std::optional<sourcemeta::core::JSON::String> id;
};

// Check misunderstood struct to be a function
// NOLINTNEXTLINE(bugprone-exception-escape)
struct CacheSubschema {
  bool orphan{};
  std::optional<sourcemeta::core::Pointer> parent{};
};

} // namespace

namespace sourcemeta::core {

auto to_json(const SchemaReferenceType value) -> JSON {
  return JSON{value == SchemaReferenceType::Static ? "static" : "dynamic"};
}

auto to_json(const SchemaFrame::LocationType value) -> JSON {
  switch (value) {
    case SchemaFrame::LocationType::Resource:
      return JSON{"resource"};
    case SchemaFrame::LocationType::Anchor:
      return JSON{"anchor"};
    case SchemaFrame::LocationType::Pointer:
      return JSON{"pointer"};
    case SchemaFrame::LocationType::Subschema:
      return JSON{"subschema"};
    default:
      assert(false);
      return JSON{nullptr};
  }
}

auto SchemaFrame::to_json(
    const std::optional<PointerPositionTracker> &tracker) const -> JSON {
  auto root{JSON::make_object()};

  root.assign_assume_new("locations", JSON::make_object());
  root.at("locations").assign_assume_new("static", JSON::make_object());
  root.at("locations").assign_assume_new("dynamic", JSON::make_object());
  for (const auto &location : this->locations_) {
    auto entry{JSON::make_object()};
    entry.assign_assume_new("parent",
                            sourcemeta::core::to_json(location.second.parent));
    entry.assign_assume_new("type",
                            sourcemeta::core::to_json(location.second.type));
    entry.assign_assume_new("root",
                            sourcemeta::core::to_json(location.second.root));
    entry.assign_assume_new("base",
                            sourcemeta::core::to_json(location.second.base));
    entry.assign_assume_new("pointer",
                            sourcemeta::core::to_json(location.second.pointer));
    if (tracker.has_value()) {
      entry.assign_assume_new(
          "position", sourcemeta::core::to_json(
                          tracker.value().get(location.second.pointer)));
    } else {
      entry.assign_assume_new("position", sourcemeta::core::to_json(nullptr));
    }

    entry.assign_assume_new(
        "relativePointer",
        sourcemeta::core::to_json(location.second.relative_pointer));
    entry.assign_assume_new("dialect",
                            sourcemeta::core::to_json(location.second.dialect));
    entry.assign_assume_new(
        "baseDialect", sourcemeta::core::to_json(location.second.base_dialect));

    switch (location.first.first) {
      case SchemaReferenceType::Static:
        root.at("locations")
            .at("static")
            .assign_assume_new(location.first.second, std::move(entry));
        break;
      case SchemaReferenceType::Dynamic:
        root.at("locations")
            .at("dynamic")
            .assign_assume_new(location.first.second, std::move(entry));
        break;
      default:
        assert(false);
    }
  }

  root.assign_assume_new("references", JSON::make_array());
  for (const auto &reference : this->references_) {
    auto entry{JSON::make_object()};
    entry.assign_assume_new("type",
                            sourcemeta::core::to_json(reference.first.first));
    entry.assign_assume_new("origin",
                            sourcemeta::core::to_json(reference.first.second));

    if (tracker.has_value()) {
      entry.assign_assume_new("position",
                              sourcemeta::core::to_json(
                                  tracker.value().get(reference.first.second)));
    } else {
      entry.assign_assume_new("position", sourcemeta::core::to_json(nullptr));
    }

    entry.assign_assume_new(
        "destination", sourcemeta::core::to_json(reference.second.destination));
    entry.assign_assume_new("base",
                            sourcemeta::core::to_json(reference.second.base));
    entry.assign_assume_new(
        "fragment", sourcemeta::core::to_json(reference.second.fragment));
    root.at("references").push_back(std::move(entry));
  }

  return root;
}

auto SchemaFrame::analyse(const JSON &root, const SchemaWalker &walker,
                          const SchemaResolver &resolver,
                          const std::optional<JSON::String> &default_dialect,
                          const std::optional<JSON::String> &default_id,
                          const SchemaFrame::Paths &paths) -> void {
  std::vector<InternalEntry> subschema_entries;
  std::unordered_map<Pointer, CacheSubschema> subschemas;
  std::unordered_map<sourcemeta::core::Pointer, std::vector<JSON::String>>
      base_uris;
  std::unordered_map<sourcemeta::core::Pointer, std::vector<JSON::String>>
      base_dialects;

  for (const auto &path : paths) {
    // Passing paths that overlap is undefined behavior. No path should
    // start with another one, else you are doing something wrong
    assert(std::ranges::all_of(paths, [&path](const auto &other) {
      return path == other || !path.starts_with(other);
    }));

    const auto &schema{get(root, path)};

    const std::optional<JSON::String> root_base_dialect{
        sourcemeta::core::base_dialect(schema, resolver, default_dialect)};
    if (!root_base_dialect.has_value()) {
      throw SchemaUnknownBaseDialectError();
    }

    std::optional<JSON::String> root_id{
        // If we are dealing with nested schemas, then by definition
        // the root has no identifier
        !path.empty() ? std::nullopt
                      : sourcemeta::core::identify(
                            schema, root_base_dialect.value(), default_id)};
    if (root_id.has_value()) {
      root_id = URI::canonicalize(root_id.value());
    }

    const std::optional<JSON::String> root_dialect{
        sourcemeta::core::dialect(schema, default_dialect)};
    assert(root_dialect.has_value());

    // If the top-level schema has a specific identifier but the user
    // passes a different default identifier, then the schema is by
    // definition known by two names, and we should handle that accordingly
    const bool has_explicit_different_id{root_id.has_value() &&
                                         default_id.has_value() &&
                                         root_id.value() != default_id.value()};
    if (has_explicit_different_id) {
      const auto default_id_canonical{URI::canonicalize(default_id.value())};
      store(this->locations_, SchemaReferenceType::Static,
            SchemaFrame::LocationType::Resource, default_id_canonical, root_id,
            root_id.value(), path, sourcemeta::core::empty_pointer,
            root_dialect.value(), root_base_dialect.value(), std::nullopt);

      base_uris.insert({path, {root_id.value(), default_id_canonical}});
    }

    std::vector<std::size_t> current_subschema_entries;
    for (const auto &relative_entry : sourcemeta::core::SchemaIterator{
             schema, walker, resolver, default_dialect}) {
      // Rephrase the iterator entry as being for the current base
      auto entry{relative_entry};
      entry.pointer = path.concat(relative_entry.pointer);
      if (entry.parent.has_value()) {
        entry.parent = path.concat(relative_entry.parent.value());
      }

      // Dialect
      assert(entry.dialect.has_value());
      base_dialects.insert({entry.pointer, {entry.dialect.value()}});

      // Base dialect
      assert(entry.base_dialect.has_value());

      // Schema identifier
      std::optional<JSON::String> id{sourcemeta::core::identify(
          entry.subschema.get(), entry.base_dialect.value(),
          entry.pointer.empty() ? root_id : std::nullopt)};

      // Store information
      subschemas.emplace(entry.pointer, CacheSubschema{.orphan = entry.orphan,
                                                       .parent = entry.parent});
      subschema_entries.emplace_back(
          InternalEntry{.common = std::move(entry), .id = std::move(id)});
      current_subschema_entries.emplace_back(subschema_entries.size() - 1);
    }

    for (const auto &entry_index : current_subschema_entries) {
      const auto &entry{subschema_entries[entry_index]};
      if (entry.id.has_value()) {
        const bool ref_overrides =
            ref_overrides_adjacent_keywords(entry.common.base_dialect.value());
        const bool is_pre_2019_09_location_independent_identifier =
            supports_id_anchors(entry.common.base_dialect.value()) &&
            sourcemeta::core::URI{entry.id.value()}.is_fragment_only();

        if ((!entry.common.subschema.get().defines("$ref") || !ref_overrides) &&
            // If we are dealing with a pre-2019-09 location independent
            // identifier, we ignore it as a traditional identifier and take
            // care of it as an anchor
            !is_pre_2019_09_location_independent_identifier) {
          const auto bases{
              find_nearest_bases(base_uris, entry.common.pointer, entry.id)};
          for (const auto &base_string : bases.first) {
            // Otherwise we end up pushing the top-level resource twice
            if (entry_index == 0 && has_explicit_different_id &&
                default_id.has_value() && default_id.value() == base_string) {
              continue;
            }

            const sourcemeta::core::URI base{base_string};
            sourcemeta::core::URI maybe_relative{entry.id.value()};
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
            maybe_relative.resolve_from(base).canonicalize();
            const JSON::String new_id{maybe_relative.recompose()};

            const auto maybe_match{
                this->locations_.find({SchemaReferenceType::Static, new_id})};
            if (maybe_match != this->locations_.cend() &&
                maybe_match->second.pointer != entry.common.pointer) {
              throw_already_exists(new_id);
            }

            if (!maybe_relative_is_absolute ||
                maybe_match == this->locations_.cend()) {
              assert(entry.common.base_dialect.has_value());

              store(this->locations_, SchemaReferenceType::Static,
                    SchemaFrame::LocationType::Resource, new_id, root_id,
                    new_id, entry.common.pointer,
                    sourcemeta::core::empty_pointer,
                    entry.common.dialect.value(),
                    entry.common.base_dialect.value(), entry.common.parent);
            }

            auto base_uri_match{base_uris.find(entry.common.pointer)};
            if (base_uri_match != base_uris.cend()) {
              if (std::find(base_uri_match->second.cbegin(),
                            base_uri_match->second.cend(),
                            new_id) == base_uri_match->second.cend()) {
                base_uri_match->second.push_back(new_id);
              }
            } else {
              base_uris.insert({entry.common.pointer, {new_id}});
            }
          }
        }
      }

      if (this->mode_ != SchemaFrame::Mode::Locations) {
        // Handle metaschema references
        const auto maybe_metaschema{
            sourcemeta::core::dialect(entry.common.subschema.get())};
        if (maybe_metaschema.has_value()) {
          sourcemeta::core::URI metaschema{maybe_metaschema.value()};
          const auto nearest_bases{
              find_nearest_bases(base_uris, entry.common.pointer, entry.id)};
          if (!nearest_bases.first.empty()) {
            metaschema.resolve_from(nearest_bases.first.front());
          }

          metaschema.canonicalize();
          const JSON::String destination{metaschema.recompose()};
          assert(entry.common.subschema.get().defines("$schema"));
          this->references_.insert_or_assign(
              {SchemaReferenceType::Static,
               entry.common.pointer.concat({"$schema"})},
              SchemaFrame::ReferencesEntry{
                  .original = maybe_metaschema.value(),
                  .destination = destination,
                  .base = metaschema.recompose_without_fragment(),
                  .fragment = fragment_string(metaschema)});
        }
      }

      // Handle schema anchors
      for (const auto &[name, type] : find_anchors(entry.common.subschema.get(),
                                                   entry.common.vocabularies)) {
        const auto bases{
            find_nearest_bases(base_uris, entry.common.pointer, entry.id)};

        if (bases.first.empty()) {
          const auto anchor_uri{sourcemeta::core::URI::from_fragment(name)};
          const auto relative_anchor_uri{anchor_uri.recompose()};

          if (type == AnchorType::Static || type == AnchorType::All) {
            store(this->locations_, SchemaReferenceType::Static,
                  SchemaFrame::LocationType::Anchor, relative_anchor_uri,
                  root_id, "", entry.common.pointer,
                  entry.common.pointer.resolve_from(bases.second),
                  entry.common.dialect.value(),
                  entry.common.base_dialect.value(), entry.common.parent);
          }

          if (type == AnchorType::Dynamic || type == AnchorType::All) {
            store(this->locations_, SchemaReferenceType::Dynamic,
                  SchemaFrame::LocationType::Anchor, relative_anchor_uri,
                  root_id, "", entry.common.pointer,
                  entry.common.pointer.resolve_from(bases.second),
                  entry.common.dialect.value(),
                  entry.common.base_dialect.value(), entry.common.parent);

            // Register a dynamic anchor as a static anchor if possible too
            if (entry.common.vocabularies.contains(
                    Vocabularies::Known::JSON_Schema_2020_12_Core)) {
              store(this->locations_, SchemaReferenceType::Static,
                    SchemaFrame::LocationType::Anchor, relative_anchor_uri,
                    root_id, "", entry.common.pointer,
                    entry.common.pointer.resolve_from(bases.second),
                    entry.common.dialect.value(),
                    entry.common.base_dialect.value(), entry.common.parent,
                    true);
            }
          }
        } else {
          bool is_first = true;
          for (const auto &base_string : bases.first) {
            sourcemeta::core::URI anchor_uri_builder{base_string};
            anchor_uri_builder.fragment(name);
            anchor_uri_builder.canonicalize();
            const auto anchor_uri{anchor_uri_builder.recompose()};

            if (!is_first && this->locations_.contains(
                                 {SchemaReferenceType::Static, anchor_uri})) {
              continue;
            }

            if (type == AnchorType::Static || type == AnchorType::All) {
              store(this->locations_,
                    sourcemeta::core::SchemaReferenceType::Static,
                    SchemaFrame::LocationType::Anchor, anchor_uri, root_id,
                    base_string, entry.common.pointer,
                    entry.common.pointer.resolve_from(bases.second),
                    entry.common.dialect.value(),
                    entry.common.base_dialect.value(), entry.common.parent);
            }

            if (type == AnchorType::Dynamic || type == AnchorType::All) {
              store(this->locations_,
                    sourcemeta::core::SchemaReferenceType::Dynamic,
                    SchemaFrame::LocationType::Anchor, anchor_uri, root_id,
                    base_string, entry.common.pointer,
                    entry.common.pointer.resolve_from(bases.second),
                    entry.common.dialect.value(),
                    entry.common.base_dialect.value(), entry.common.parent);

              // Register a dynamic anchor as a static anchor if possible too
              if (entry.common.vocabularies.contains(
                      Vocabularies::Known::JSON_Schema_2020_12_Core)) {
                store(this->locations_,
                      sourcemeta::core::SchemaReferenceType::Static,
                      SchemaFrame::LocationType::Anchor, anchor_uri, root_id,
                      base_string, entry.common.pointer,
                      entry.common.pointer.resolve_from(bases.second),
                      entry.common.dialect.value(),
                      entry.common.base_dialect.value(), entry.common.parent,
                      true);
              }
            }

            is_first = false;
          }
        }
      }
    }

    // It is important for the loop that follows to assume a specific ordering
    // where smaller pointers (by number of tokens) are scanned first.
    // TODO: Perform the pointer walking using weak pointers only
    const auto pointer_walker{sourcemeta::core::PointerWalker{schema}};
    std::vector<sourcemeta::core::Pointer> pointers{pointer_walker.cbegin(),
                                                    pointer_walker.cend()};
    std::ranges::sort(pointers, std::less<>());

    // Pre-compute every possible pointer to the schema
    for (const auto &relative_pointer : pointers) {
      const auto pointer{path.concat(relative_pointer)};

      const auto dialects{
          find_nearest_bases(base_dialects, pointer, root_dialect)};
      assert(dialects.first.size() == 1);

      auto every_base_result = find_every_base(base_uris, pointer);

      for (const auto &base : every_base_result) {
        auto relative_pointer_uri{
            base.first.empty()
                ? sourcemeta::core::to_uri(pointer.resolve_from(base.second))
                : sourcemeta::core::to_uri(pointer.resolve_from(base.second))
                      .resolve_from({base.first})};

        relative_pointer_uri.canonicalize();
        auto result{relative_pointer_uri.recompose()};

        bool contains =
            this->locations_.contains({SchemaReferenceType::Static, result});

        if (!contains) {
          const auto nearest_bases{
              find_nearest_bases(base_uris, pointer, base.first)};
          assert(!nearest_bases.first.empty());
          const auto &current_base{nearest_bases.first.front()};

          const auto maybe_base_entry{this->locations_.find(
              {SchemaReferenceType::Static, current_base})};

          const auto current_base_dialect{
              maybe_base_entry == this->locations_.cend()
                  ? root_base_dialect.value()
                  : maybe_base_entry->second.base_dialect};

          const auto subschema{subschemas.find(pointer)};

          if (subschema != subschemas.cend()) {
            store(this->locations_, SchemaReferenceType::Static,
                  SchemaFrame::LocationType::Subschema, result, root_id,
                  current_base, pointer,
                  pointer.resolve_from(nearest_bases.second),
                  dialects.first.front(), current_base_dialect,
                  subschema->second.parent, false, true);
          } else {
            store(this->locations_, SchemaReferenceType::Static,
                  SchemaFrame::LocationType::Pointer, result, root_id,
                  current_base, pointer,
                  pointer.resolve_from(nearest_bases.second),
                  dialects.first.front(), current_base_dialect, dialects.second,
                  false, true);
          }
        }
      }
    }
  }

  if (this->mode_ == SchemaFrame::Mode::Locations) {
    return;
  }

  // Resolve references after all framing was performed
  for (const auto &entry : subschema_entries) {
    if (entry.common.subschema.get().is_object()) {
      const auto nearest_bases{
          find_nearest_bases(base_uris, entry.common.pointer, entry.id)};
      if (entry.common.subschema.get().defines("$ref")) {
        if (entry.common.subschema.get().at("$ref").is_string()) {
          const auto &original{
              entry.common.subschema.get().at("$ref").to_string()};
          sourcemeta::core::URI ref{original};
          if (!nearest_bases.first.empty()) {
            ref.resolve_from(nearest_bases.first.front());
          }

          ref.canonicalize();
          this->references_.insert_or_assign(
              {SchemaReferenceType::Static,
               entry.common.pointer.concat({"$ref"})},
              SchemaFrame::ReferencesEntry{.original = original,
                                           .destination = ref.recompose(),
                                           .base =
                                               ref.recompose_without_fragment(),
                                           .fragment = fragment_string(ref)});
        }
      }

      if (entry.common.vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2019_09_Core) &&
          entry.common.subschema.get().defines("$recursiveRef")) {
        assert(entry.common.subschema.get().at("$recursiveRef").is_string());
        const auto &ref{
            entry.common.subschema.get().at("$recursiveRef").to_string()};

        // The behavior of this keyword is defined only for the value "#".
        // Implementations MAY choose to consider other values to be errors.
        // See
        // https://json-schema.org/draft/2019-09/draft-handrews-json-schema-02#rfc.section.8.2.4.2.1
        if (ref != "#") {
          throw sourcemeta::core::SchemaReferenceError(
              entry.id.value_or(""),
              entry.common.pointer.concat({"$recursiveRef"}),
              "Invalid recursive reference");
        }

        auto anchor_uri_string{
            nearest_bases.first.empty() ? "" : nearest_bases.first.front()};
        const auto recursive_anchor{this->locations_.find(
            {SchemaReferenceType::Dynamic, anchor_uri_string})};
        const auto reference_type{recursive_anchor == this->locations_.end()
                                      ? SchemaReferenceType::Static
                                      : SchemaReferenceType::Dynamic};
        const sourcemeta::core::URI anchor_uri{anchor_uri_string};
        this->references_.insert_or_assign(
            {reference_type, entry.common.pointer.concat({"$recursiveRef"})},
            SchemaFrame::ReferencesEntry{
                .original = ref,
                .destination = anchor_uri.recompose(),
                .base = anchor_uri.recompose_without_fragment(),
                .fragment = fragment_string(anchor_uri)});
      }

      if (entry.common.vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2020_12_Core) &&
          entry.common.subschema.get().defines("$dynamicRef")) {
        if (entry.common.subschema.get().at("$dynamicRef").is_string()) {
          const auto &original{
              entry.common.subschema.get().at("$dynamicRef").to_string()};
          sourcemeta::core::URI ref{original};
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
              this->locations_.find({SchemaReferenceType::Static, ref_string})};
          const auto maybe_dynamic_frame{this->locations_.find(
              {SchemaReferenceType::Dynamic, ref_string})};
          const auto behaves_as_static{
              !has_fragment ||
              (has_fragment && maybe_static_frame != this->locations_.end() &&
               maybe_dynamic_frame == this->locations_.end())};
          this->references_.insert_or_assign(
              {behaves_as_static ? SchemaReferenceType::Static
                                 : SchemaReferenceType::Dynamic,
               entry.common.pointer.concat({"$dynamicRef"})},
              SchemaFrame::ReferencesEntry{.original = original,
                                           .destination = std::move(ref_string),
                                           .base =
                                               ref.recompose_without_fragment(),
                                           .fragment = fragment_string(ref)});
        }
      }
    }
  }

  // A schema is standalone if all references can be resolved within itself
  if (this->standalone()) {
    // Find all dynamic anchors
    std::map<JSON::String, std::vector<JSON::String>> dynamic_anchors;
    for (const auto &entry : this->locations_) {
      if (entry.first.first != SchemaReferenceType::Dynamic ||
          entry.second.type != SchemaFrame::LocationType::Anchor) {
        continue;
      }

      const URI anchor_uri{entry.first.second};
      const JSON::String fragment{anchor_uri.fragment().value_or("")};
      if (!dynamic_anchors.contains(fragment)) {
        dynamic_anchors.emplace(fragment, std::vector<JSON::String>{});
      }

      dynamic_anchors[fragment].push_back(entry.first.second);
    }

    // If there is a dynamic reference that only has one possible
    // dynamic anchor destination, then that dynamic reference
    // is a static reference in disguise
    std::vector<SchemaFrame::References::key_type> to_delete;
    std::vector<SchemaFrame::References::value_type> to_insert;
    for (const auto &reference : this->references_) {
      if (reference.first.first != SchemaReferenceType::Dynamic ||
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
          SchemaFrame::References::key_type{SchemaReferenceType::Static,
                                            reference.first.second},
          SchemaFrame::References::mapped_type{
              match->second.front(), match->second.front(),
              new_destination.recompose_without_fragment(),
              fragment_string(new_destination)});
    }

    // Because we can't mutate a map as we are traversing it

    for (const auto &key : to_delete) {
      this->references_.erase(key);
    }

    for (auto &&entry : to_insert) {
      this->references_.emplace(std::move(entry));
    }
  }
}

auto SchemaFrame::locations() const noexcept -> const Locations & {
  return this->locations_;
}

auto SchemaFrame::references() const noexcept -> const References & {
  return this->references_;
}

auto SchemaFrame::standalone() const -> bool {
  return std::ranges::all_of(this->references_, [&](const auto &reference) {
    assert(!reference.first.second.empty());
    assert(reference.first.second.back().is_property());
    // TODO: This check might need to be more elaborate given
    // https://github.com/sourcemeta/core/issues/1390
    return reference.first.second.back().to_property() == "$schema" ||
           this->locations_.contains(
               {SchemaReferenceType::Static, reference.second.destination}) ||
           this->locations_.contains(
               {SchemaReferenceType::Dynamic, reference.second.destination});
  });
}

auto SchemaFrame::vocabularies(const Location &location,
                               const SchemaResolver &resolver) const
    -> Vocabularies {
  return sourcemeta::core::vocabularies(resolver, location.base_dialect,
                                        location.dialect);
}

auto SchemaFrame::uri(const Location &location,
                      const Pointer &relative_schema_location) const
    -> JSON::String {
  return to_uri(location.relative_pointer.concat(relative_schema_location),
                location.base)
      .recompose();
}

auto SchemaFrame::traverse(const Location &location,
                           const Pointer &relative_schema_location) const
    -> const Location & {
  const auto new_uri{this->uri(location, relative_schema_location)};
  const auto static_match{
      this->locations_.find({SchemaReferenceType::Static, new_uri})};
  if (static_match != this->locations_.cend()) {
    return static_match->second;
  }

  const auto dynamic_match{
      this->locations_.find({SchemaReferenceType::Dynamic, new_uri})};
  assert(dynamic_match != this->locations_.cend());
  return dynamic_match->second;
}

auto SchemaFrame::traverse(const JSON::String &uri) const
    -> std::optional<std::reference_wrapper<const Location>> {
  const auto static_result{
      this->locations_.find({SchemaReferenceType::Static, uri})};
  if (static_result != this->locations_.cend()) {
    return static_result->second;
  }

  const auto dynamic_result{
      this->locations_.find({SchemaReferenceType::Dynamic, uri})};
  if (dynamic_result != this->locations_.cend()) {
    return dynamic_result->second;
  }

  return std::nullopt;
}

auto SchemaFrame::uri(const Pointer &pointer) const
    -> std::optional<std::reference_wrapper<const JSON::String>> {
  // TODO: This is potentially very slow. Traversing by pointer shouldn't
  // require an O(N) operation
  for (const auto &entry : this->locations_) {
    if (entry.second.pointer == pointer) {
      return entry.first.second;
    }
  }

  return std::nullopt;
}

auto SchemaFrame::dereference(const Location &location,
                              const Pointer &relative_schema_location) const
    -> std::pair<SchemaReferenceType,
                 std::optional<std::reference_wrapper<const Location>>> {
  const auto effective_location{
      location.pointer.concat({relative_schema_location})};
  const auto maybe_reference_entry{this->references_.find(
      {SchemaReferenceType::Static, effective_location})};
  if (maybe_reference_entry == this->references_.cend()) {
    // If static dereferencing failed but we know the reference
    // is dynamic, then report so, but without a location, as by
    // definition we can't know the destination until at runtime
    if (this->references_.contains(
            {SchemaReferenceType::Dynamic, effective_location})) {
      return {SchemaReferenceType::Dynamic, std::nullopt};
    }

    return {SchemaReferenceType::Static, std::nullopt};
  }

  const auto destination{
      this->locations_.find({SchemaReferenceType::Static,
                             maybe_reference_entry->second.destination})};
  assert(destination != this->locations_.cend());
  return {SchemaReferenceType::Static, destination->second};
}

auto SchemaFrame::references_to(const Pointer &pointer) const -> std::vector<
    std::reference_wrapper<const typename References::value_type>> {
  std::vector<std::reference_wrapper<const typename References::value_type>>
      result;

  // TODO: This is currently very slow, as we need to loop on every reference
  // to brute force whether it points to the desired entry or not
  for (const auto &reference : this->references_) {
    assert(!reference.first.second.empty());
    assert(reference.first.second.back().is_property());

    if (reference.first.first == SchemaReferenceType::Static) {
      const auto match{this->locations_.find(
          {reference.first.first, reference.second.destination})};
      if (match != this->locations_.cend() &&
          match->second.pointer == pointer) {
        result.emplace_back(reference);
      }
    } else {
      for (const auto &location : this->locations_) {
        if (location.second.type == LocationType::Anchor &&
            location.first.first == SchemaReferenceType::Dynamic &&
            location.second.pointer == pointer) {
          if (!reference.second.fragment.has_value() ||
              URI{location.first.second}.fragment().value_or("") ==
                  reference.second.fragment.value()) {
            result.emplace_back(reference);
          }
        }
      }
    }
  }

  return result;
}

} // namespace sourcemeta::core
