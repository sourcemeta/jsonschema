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
    -> std::vector<std::pair<std::string_view, AnchorType>> {
  std::vector<std::pair<std::string_view, AnchorType>> result;

  // 2020-12
  if (schema.is_object() &&
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_2020_12_Core)) {
    if (schema.defines("$dynamicAnchor")) {
      const auto &anchor{schema.at("$dynamicAnchor")};
      if (anchor.is_string()) {
        result.emplace_back(anchor.to_string(), AnchorType::Dynamic);
      }
    }

    if (schema.defines("$anchor")) {
      const auto &anchor{schema.at("$anchor")};
      if (anchor.is_string()) {
        const std::string_view anchor_view{anchor.to_string()};
        bool found = false;
        for (auto &entry : result) {
          if (entry.first == anchor_view) {
            entry.second = AnchorType::All;
            found = true;
            break;
          }
        }
        if (!found) {
          result.emplace_back(anchor_view, AnchorType::Static);
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
        result.emplace_back(std::string_view{}, AnchorType::Dynamic);
      }
    }

    if (schema.defines("$anchor")) {
      const auto &anchor{schema.at("$anchor")};
      if (anchor.is_string()) {
        const std::string_view anchor_view{anchor.to_string()};
        bool found = false;
        for (auto &entry : result) {
          if (entry.first == anchor_view) {
            entry.second = AnchorType::All;
            found = true;
            break;
          }
        }
        if (!found) {
          result.emplace_back(anchor_view, AnchorType::Static);
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
      const auto &id_string{schema.at("$id").to_string()};
      if (id_string.starts_with('#')) {
        // The original string is "#fragment", skip the '#'
        result.emplace_back(std::string_view{id_string}.substr(1),
                            AnchorType::Static);
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
      const auto &id_string{schema.at("id").to_string()};
      if (id_string.starts_with('#')) {
        // The original string is "#fragment", skip the '#'
        result.emplace_back(std::string_view{id_string}.substr(1),
                            AnchorType::Static);
      }
    }
  }

  return result;
}

template <typename StringType>
auto find_nearest_bases_ref(
    const std::unordered_map<sourcemeta::core::WeakPointer,
                             std::vector<StringType>> &bases,
    const sourcemeta::core::WeakPointer &pointer)
    -> std::optional<
        std::pair<std::reference_wrapper<const std::vector<StringType>>,
                  sourcemeta::core::WeakPointer>> {
  auto current_pointer{pointer};
  while (true) {
    const auto match{bases.find(current_pointer)};
    if (match != bases.cend()) {
      return std::make_pair(std::cref(match->second), current_pointer);
    }

    if (current_pointer.empty()) {
      break;
    }

    current_pointer = current_pointer.initial();
  }

  return std::nullopt;
}

template <typename StringType>
auto find_nearest_bases(
    const std::unordered_map<sourcemeta::core::WeakPointer,
                             std::vector<StringType>> &bases,
    const sourcemeta::core::WeakPointer &pointer,
    const std::optional<std::string_view> &default_base)
    -> std::pair<std::vector<StringType>, sourcemeta::core::WeakPointer> {
  const auto result{find_nearest_bases_ref(bases, pointer)};
  if (result.has_value()) {
    return {result->first.get(), result->second};
  }

  if (default_base.has_value()) {
    return {{StringType{default_base.value()}},
            sourcemeta::core::empty_weak_pointer};
  }

  return {{}, sourcemeta::core::empty_weak_pointer};
}

auto find_every_base(
    const std::unordered_map<sourcemeta::core::WeakPointer,
                             std::vector<sourcemeta::core::JSON::String>>
        &bases,
    const sourcemeta::core::WeakPointer &pointer)
    -> std::vector<std::pair<std::string_view, sourcemeta::core::WeakPointer>> {
  std::vector<std::pair<std::string_view, sourcemeta::core::WeakPointer>>
      result;

  auto current_pointer{pointer};
  while (true) {
    const auto match{bases.find(current_pointer)};
    if (match != bases.cend()) {
      for (const auto &base : match->second) {
        result.emplace_back(std::string_view{base}, current_pointer);
      }
    }

    if (current_pointer.empty()) {
      break;
    }

    current_pointer = current_pointer.initial();
  }

  if (result.empty() ||
      result.back().second != sourcemeta::core::empty_weak_pointer) {
    result.emplace_back(std::string_view{},
                        sourcemeta::core::empty_weak_pointer);
  }

  return result;
}

// TODO: Why do we have this function both here and on `walker.cc`?
auto ref_overrides_adjacent_keywords(
    const sourcemeta::core::SchemaBaseDialect base_dialect) -> bool {
  using sourcemeta::core::SchemaBaseDialect;
  // In older drafts, the presence of `$ref` would override any sibling
  // keywords
  // See
  // https://json-schema.org/draft-07/draft-handrews-json-schema-01#rfc.section.8.3
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_Schema_Draft_7:
    case SchemaBaseDialect::JSON_Schema_Draft_7_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_6:
    case SchemaBaseDialect::JSON_Schema_Draft_6_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_4:
    case SchemaBaseDialect::JSON_Schema_Draft_4_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_3:
    case SchemaBaseDialect::JSON_Schema_Draft_3_Hyper:
      return true;
    default:
      return false;
  }
}

auto supports_id_anchors(const sourcemeta::core::SchemaBaseDialect base_dialect)
    -> bool {
  using sourcemeta::core::SchemaBaseDialect;
  switch (base_dialect) {
    case SchemaBaseDialect::JSON_Schema_Draft_7:
    case SchemaBaseDialect::JSON_Schema_Draft_7_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_6:
    case SchemaBaseDialect::JSON_Schema_Draft_6_Hyper:
    case SchemaBaseDialect::JSON_Schema_Draft_4:
    case SchemaBaseDialect::JSON_Schema_Draft_4_Hyper:
      return true;
    default:
      return false;
  }
}

auto set_base_and_fragment(
    sourcemeta::core::SchemaFrame::ReferencesEntry &entry) -> void {
  if (entry.destination.empty()) {
    entry.base = std::string_view{};
    entry.fragment = std::nullopt;
    return;
  }

  const auto hash_position{entry.destination.find('#')};
  if (hash_position != std::string::npos) {
    // Has a fragment
    if (hash_position == 0) {
      // Starts with #, so no base
      entry.base = std::string_view{};
    } else {
      entry.base = std::string_view{entry.destination}.substr(0, hash_position);
    }
    entry.fragment =
        std::string_view{entry.destination}.substr(hash_position + 1);
  } else {
    // No fragment
    entry.base = std::string_view{entry.destination};
    entry.fragment = std::nullopt;
  }
}

[[noreturn]]
auto throw_already_exists(const sourcemeta::core::JSON::String &uri) -> void {
  throw sourcemeta::core::SchemaFrameError(uri,
                                           "Schema identifier already exists");
}

auto store(sourcemeta::core::SchemaFrame::Locations &frame,
           const sourcemeta::core::SchemaReferenceType type,
           const sourcemeta::core::SchemaFrame::LocationType entry_type,
           sourcemeta::core::JSON::String uri, const std::string_view base,
           const sourcemeta::core::WeakPointer &pointer_from_root,
           const std::size_t relative_pointer_offset,
           const std::string_view dialect,
           const sourcemeta::core::SchemaBaseDialect base_dialect,
           const std::optional<sourcemeta::core::WeakPointer> &parent,
           const bool ignore_if_present = false,
           const bool already_canonical = false) -> void {
  auto canonical{already_canonical ? std::move(uri)
                                   : sourcemeta::core::URI::canonicalize(uri)};
  auto [iterator, inserted] =
      frame.insert({{type, std::move(canonical)},
                    {.parent = parent,
                     .type = entry_type,
                     .base = base,
                     .pointer = to_pointer(pointer_from_root),
                     .relative_pointer = relative_pointer_offset,
                     .dialect = dialect,
                     .base_dialect = base_dialect}});
  if (!ignore_if_present && !inserted) {
    throw_already_exists(iterator->first.second);
  }

  if (inserted && iterator->first.second == base) {
    iterator->second.base = iterator->first.second;
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
  std::optional<sourcemeta::core::WeakPointer> parent{};
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
    entry.assign_assume_new("root", this->root_.empty() ? JSON{nullptr}
                                                        : JSON{this->root_});
    entry.assign_assume_new("base", JSON{JSON::String{location.second.base}});
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
        sourcemeta::core::to_json(
            this->relative_instance_location(location.second)));
    entry.assign_assume_new("dialect",
                            JSON{JSON::String{location.second.dialect}});
    entry.assign_assume_new(
        "baseDialect",
        JSON{JSON::String{to_string(location.second.base_dialect)}});

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
    entry.assign_assume_new(
        "base",
        !reference.second.base.empty()
            ? sourcemeta::core::to_json(JSON::String{reference.second.base})
            : sourcemeta::core::to_json(nullptr));
    entry.assign_assume_new(
        "fragment", reference.second.fragment.has_value()
                        ? sourcemeta::core::to_json(
                              JSON::String{reference.second.fragment.value()})
                        : sourcemeta::core::to_json(nullptr));
    root.at("references").push_back(std::move(entry));
  }

  return root;
}

auto SchemaFrame::analyse(const JSON &root, const SchemaWalker &walker,
                          const SchemaResolver &resolver,
                          std::string_view default_dialect,
                          std::string_view default_id,
                          const SchemaFrame::Paths &paths) -> void {
  this->reset();
  assert(std::unordered_set<WeakPointer>(paths.cbegin(), paths.cend()).size() ==
         paths.size());
  std::vector<InternalEntry> subschema_entries;
  std::unordered_map<WeakPointer, CacheSubschema> subschemas;
  std::unordered_map<WeakPointer, std::vector<JSON::String>> base_uris;
  std::unordered_map<WeakPointer, std::vector<std::string_view>> base_dialects;

  for (const auto &path : paths) {
    // Passing paths that overlap is undefined behavior. No path should
    // start with another one, else you are doing something wrong
    assert(std::ranges::all_of(paths, [&path](const auto &other) {
      return path == other || !path.starts_with(other);
    }));

    const auto &schema{get(root, path)};

    const auto root_base_dialect{
        sourcemeta::core::base_dialect(schema, resolver, default_dialect)};
    if (!root_base_dialect.has_value()) {
      throw SchemaUnknownBaseDialectError();
    }

    // If we are dealing with nested schemas, then by definition
    // the root has no identifier
    std::optional<JSON::String> root_id{std::nullopt};
    if (path.empty()) {
      const auto maybe_id{sourcemeta::core::identify(
          schema, root_base_dialect.value(), default_id)};
      if (!maybe_id.empty()) {
        root_id = URI::canonicalize(maybe_id);
        this->root_ = root_id.value();
      }
    }

    const std::string_view root_dialect{
        sourcemeta::core::dialect(schema, default_dialect)};
    assert(!root_dialect.empty());

    // If the top-level schema has a specific identifier but the user
    // passes a different default identifier, then the schema is by
    // definition known by two names, and we should handle that accordingly
    const bool has_explicit_different_id{root_id.has_value() &&
                                         !default_id.empty() &&
                                         root_id.value() != default_id};
    if (has_explicit_different_id) {
      const auto default_id_canonical{URI::canonicalize(default_id)};
      // Use this->root_ as base - it contains root_id.value() and persists
      store(this->locations_, SchemaReferenceType::Static,
            SchemaFrame::LocationType::Resource, default_id_canonical,
            this->root_, path, path.size(), root_dialect,
            root_base_dialect.value(), std::nullopt);

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
      assert(!entry.dialect.empty());
      base_dialects.insert({entry.pointer, {entry.dialect}});

      // Base dialect
      assert(entry.base_dialect.has_value());

      // Schema identifier
      // We need to store the default_id in a local variable to ensure
      // it survives the identify() call, as identify() returns a string_view
      const std::string default_id_for_entry{
          entry.pointer.empty() && root_id.has_value() ? root_id.value()
                                                       : std::string{}};
      const auto maybe_id{sourcemeta::core::identify(entry.subschema.get(),
                                                     entry.base_dialect.value(),
                                                     default_id_for_entry)};
      std::optional<JSON::String> id{
          !maybe_id.empty()
              ? std::make_optional<JSON::String>(std::string{maybe_id})
              : std::nullopt};

      // Store information
      subschemas.emplace(entry.pointer, CacheSubschema{.orphan = entry.orphan,
                                                       .parent = entry.parent});
      subschema_entries.emplace_back(
          InternalEntry{.common = std::move(entry), .id = std::move(id)});
      current_subschema_entries.emplace_back(subschema_entries.size() - 1);
    }

    for (const auto &entry_index : current_subschema_entries) {
      const auto &entry{subschema_entries[entry_index]};
      const auto &common_pointer_weak{entry.common.pointer};
      const auto common_pointer{to_pointer(common_pointer_weak)};
      const auto &common_parent{entry.common.parent};
      if (entry.id.has_value()) {
        assert(entry.common.base_dialect.has_value());
        const bool ref_overrides =
            ref_overrides_adjacent_keywords(entry.common.base_dialect.value());
        const bool is_pre_2019_09_location_independent_identifier =
            supports_id_anchors(entry.common.base_dialect.value()) &&
            entry.id.value().starts_with('#');

        if ((!entry.common.subschema.get().defines("$ref") || !ref_overrides) &&
            // If we are dealing with a pre-2019-09 location independent
            // identifier, we ignore it as a traditional identifier and take
            // care of it as an anchor
            !is_pre_2019_09_location_independent_identifier) {
          const auto bases{find_nearest_bases(
              base_uris, common_pointer_weak,
              entry.id ? std::optional<std::string_view>{*entry.id}
                       : std::nullopt)};
          for (const auto &base_string : bases.first) {
            // Otherwise we end up pushing the top-level resource twice
            if (entry_index == 0 && has_explicit_different_id &&
                !default_id.empty() && default_id == base_string) {
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
                maybe_match->second.pointer != common_pointer) {
              throw_already_exists(new_id);
            }

            if (!maybe_relative_is_absolute ||
                maybe_match == this->locations_.cend()) {
              assert(entry.common.base_dialect.has_value());

              store(this->locations_, SchemaReferenceType::Static,
                    SchemaFrame::LocationType::Resource, new_id, new_id,
                    common_pointer_weak, common_pointer_weak.size(),
                    entry.common.dialect, entry.common.base_dialect.value(),
                    common_parent);
            }

            auto base_uri_match{base_uris.find(common_pointer_weak)};
            if (base_uri_match != base_uris.cend()) {
              if (std::find(base_uri_match->second.cbegin(),
                            base_uri_match->second.cend(),
                            new_id) == base_uri_match->second.cend()) {
                base_uri_match->second.push_back(new_id);
              }
            } else {
              base_uris.insert({common_pointer_weak, {new_id}});
            }
          }
        }
      }

      if (this->mode_ != SchemaFrame::Mode::Locations) {
        // Handle metaschema references
        const auto maybe_metaschema{
            sourcemeta::core::dialect(entry.common.subschema.get())};
        if (!maybe_metaschema.empty()) {
          sourcemeta::core::URI metaschema{maybe_metaschema};
          const auto nearest_bases{find_nearest_bases(
              base_uris, common_pointer_weak,
              entry.id ? std::optional<std::string_view>{*entry.id}
                       : std::nullopt)};
          if (!nearest_bases.first.empty()) {
            metaschema.resolve_from(nearest_bases.first.front());
          }

          metaschema.canonicalize();
          assert(entry.common.subschema.get().defines("$schema"));
          const auto [it, inserted] = this->references_.insert_or_assign(
              {SchemaReferenceType::Static, common_pointer.concat({"$schema"})},
              SchemaFrame::ReferencesEntry{.original = maybe_metaschema,
                                           .destination =
                                               metaschema.recompose(),
                                           .base = std::string_view{},
                                           .fragment = std::nullopt});
          set_base_and_fragment(it->second);
        }
      }

      // Handle schema anchors
      for (const auto &[name, type] : find_anchors(entry.common.subschema.get(),
                                                   entry.common.vocabularies)) {
        const auto bases{find_nearest_bases(
            base_uris, common_pointer_weak,
            entry.id ? std::optional<std::string_view>{*entry.id}
                     : std::nullopt)};

        if (bases.first.empty()) {
          const auto anchor_uri{sourcemeta::core::URI::from_fragment(name)};
          const auto relative_anchor_uri{anchor_uri.recompose()};

          if (type == AnchorType::Static || type == AnchorType::All) {
            store(this->locations_, SchemaReferenceType::Static,
                  SchemaFrame::LocationType::Anchor, relative_anchor_uri, "",
                  common_pointer_weak, bases.second.size(),
                  entry.common.dialect, entry.common.base_dialect.value(),
                  common_parent);
          }

          if (type == AnchorType::Dynamic || type == AnchorType::All) {
            store(this->locations_, SchemaReferenceType::Dynamic,
                  SchemaFrame::LocationType::Anchor, relative_anchor_uri, "",
                  common_pointer_weak, bases.second.size(),
                  entry.common.dialect, entry.common.base_dialect.value(),
                  common_parent);

            // Register a dynamic anchor as a static anchor if possible too
            if (entry.common.vocabularies.contains(
                    Vocabularies::Known::JSON_Schema_2020_12_Core)) {
              store(this->locations_, SchemaReferenceType::Static,
                    SchemaFrame::LocationType::Anchor, relative_anchor_uri, "",
                    common_pointer_weak, bases.second.size(),
                    entry.common.dialect, entry.common.base_dialect.value(),
                    common_parent, true);
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

            const auto base_entry{this->locations_.find(
                {SchemaReferenceType::Static, base_string})};

            const std::string_view base_view{
                base_entry != this->locations_.cend()
                    ? std::string_view{base_entry->first.second}
                    : std::string_view{base_string}};

            if (type == AnchorType::Static || type == AnchorType::All) {
              store(this->locations_,
                    sourcemeta::core::SchemaReferenceType::Static,
                    SchemaFrame::LocationType::Anchor, anchor_uri, base_view,
                    common_pointer_weak, bases.second.size(),
                    entry.common.dialect, entry.common.base_dialect.value(),
                    common_parent);
            }

            if (type == AnchorType::Dynamic || type == AnchorType::All) {
              store(this->locations_,
                    sourcemeta::core::SchemaReferenceType::Dynamic,
                    SchemaFrame::LocationType::Anchor, anchor_uri, base_view,
                    common_pointer_weak, bases.second.size(),
                    entry.common.dialect, entry.common.base_dialect.value(),
                    common_parent);

              // Register a dynamic anchor as a static anchor if possible too
              if (entry.common.vocabularies.contains(
                      Vocabularies::Known::JSON_Schema_2020_12_Core)) {
                store(this->locations_,
                      sourcemeta::core::SchemaReferenceType::Static,
                      SchemaFrame::LocationType::Anchor, anchor_uri, base_view,
                      common_pointer_weak, bases.second.size(),
                      entry.common.dialect, entry.common.base_dialect.value(),
                      common_parent, true);
              }
            }

            is_first = false;
          }
        }
      }
    }

    // It is important for the loop that follows to assume a specific ordering
    // where smaller pointers (by number of tokens) are scanned first.
    std::vector<sourcemeta::core::WeakPointer> pointers;
    for (const auto &weak_pointer : sourcemeta::core::PointerWalker{schema}) {
      pointers.push_back(weak_pointer);
    }

    std::ranges::sort(pointers, std::less<>());

    // Pre-compute every possible pointer to the schema
    for (const auto &relative_pointer : pointers) {
      const auto pointer_weak{path.concat(relative_pointer)};

      const auto dialect_match{
          find_nearest_bases_ref(base_dialects, pointer_weak)};
      const auto &dialect_for_pointer{dialect_match.has_value()
                                          ? dialect_match->first.get().front()
                                          : root_dialect};

      auto every_base_result = find_every_base(base_uris, pointer_weak);

      WeakPointer cached_base{};
      for (const auto &base : every_base_result) {
        const auto resolved{cached_base == base.second
                                ? pointer_weak.resolve_from(cached_base)
                                : pointer_weak.resolve_from(base.second)};
        cached_base = base.second;

        auto relative_pointer_uri{
            base.first.empty()
                ? sourcemeta::core::to_uri(resolved)
                : sourcemeta::core::to_uri(resolved, base.first)};

        relative_pointer_uri.canonicalize();
        auto result{relative_pointer_uri.recompose()};

        bool contains =
            this->locations_.contains({SchemaReferenceType::Static, result});

        if (!contains) {
          const auto nearest_bases{
              find_nearest_bases(base_uris, pointer_weak,
                                 std::optional<std::string_view>{base.first})};
          assert(!nearest_bases.first.empty());
          const auto &current_base{nearest_bases.first.front()};

          const auto base_entry{this->locations_.find(
              {SchemaReferenceType::Static, current_base})};

          const std::string_view base_view{
              base_entry != this->locations_.cend()
                  ? std::string_view{base_entry->first.second}
                  : std::string_view{current_base}};

          const sourcemeta::core::SchemaBaseDialect current_base_dialect{
              base_entry != this->locations_.cend()
                  ? base_entry->second.base_dialect
                  : root_base_dialect.value()};

          const auto subschema{subschemas.find(pointer_weak)};
          if (subschema != subschemas.cend()) {
            store(this->locations_, SchemaReferenceType::Static,
                  SchemaFrame::LocationType::Subschema, std::move(result),
                  base_view, pointer_weak, nearest_bases.second.size(),
                  dialect_for_pointer, current_base_dialect,
                  subschema->second.parent, false, true);
          } else {
            store(this->locations_, SchemaReferenceType::Static,
                  SchemaFrame::LocationType::Pointer, std::move(result),
                  base_view, pointer_weak, nearest_bases.second.size(),
                  dialect_for_pointer, current_base_dialect,
                  dialect_match.has_value() ? dialect_match->second
                                            : empty_weak_pointer,
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
    const auto &common_pointer_weak{entry.common.pointer};
    const auto common_pointer{to_pointer(common_pointer_weak)};
    if (entry.common.subschema.get().is_object()) {
      const auto nearest_bases{find_nearest_bases(
          base_uris, common_pointer_weak,
          entry.id ? std::optional<std::string_view>{*entry.id}
                   : std::nullopt)};
      if (entry.common.subschema.get().defines("$ref")) {
        if (entry.common.subschema.get().at("$ref").is_string()) {
          const auto &original{
              entry.common.subschema.get().at("$ref").to_string()};
          sourcemeta::core::URI ref{original};
          if (!nearest_bases.first.empty()) {
            ref.resolve_from(nearest_bases.first.front());
          }

          ref.canonicalize();
          const auto [it, inserted] = this->references_.insert_or_assign(
              {SchemaReferenceType::Static, common_pointer.concat({"$ref"})},
              SchemaFrame::ReferencesEntry{.original = original,
                                           .destination = ref.recompose(),
                                           .base = std::string_view{},
                                           .fragment = std::nullopt});
          set_base_and_fragment(it->second);
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
              entry.id.value_or(""), common_pointer.concat({"$recursiveRef"}),
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
        const auto [it, inserted] = this->references_.insert_or_assign(
            {reference_type, common_pointer.concat({"$recursiveRef"})},
            SchemaFrame::ReferencesEntry{.original = ref,
                                         .destination = anchor_uri.recompose(),
                                         .base = std::string_view{},
                                         .fragment = std::nullopt});
        set_base_and_fragment(it->second);
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
          const auto [it, inserted] = this->references_.insert_or_assign(
              {behaves_as_static ? SchemaReferenceType::Static
                                 : SchemaReferenceType::Dynamic,
               common_pointer.concat({"$dynamicRef"})},
              SchemaFrame::ReferencesEntry{.original = original,
                                           .destination = std::move(ref_string),
                                           .base = std::string_view{},
                                           .fragment = std::nullopt});
          set_base_and_fragment(it->second);
        }
      }
    }
  }

  // A schema is standalone if all references can be resolved within itself
  if (this->standalone()) {
    // Find all dynamic anchors
    // Values are pointers to full URIs in locations_
    std::unordered_map<JSON::String, std::vector<const JSON::String *>>
        dynamic_anchors;
    for (const auto &entry : this->locations_) {
      if (entry.first.first != SchemaReferenceType::Dynamic ||
          entry.second.type != SchemaFrame::LocationType::Anchor) {
        continue;
      }

      const URI anchor_uri{entry.first.second};
      // Copy the fragment to avoid dangling string_view (anchor_uri is local)
      const JSON::String fragment{anchor_uri.fragment().value_or("")};
      dynamic_anchors[fragment].push_back(&entry.first.second);
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

      const auto match{dynamic_anchors.find(
          JSON::String{reference.second.fragment.value()})};
      assert(match != dynamic_anchors.cend());
      // Otherwise we can assume there is only one possible target for the
      // dynamic reference
      if (match->second.size() != 1) {
        continue;
      }

      to_delete.push_back(reference.first);
      to_insert.emplace_back(
          SchemaFrame::References::key_type{SchemaReferenceType::Static,
                                            reference.first.second},
          SchemaFrame::References::mapped_type{
              reference.second.original, *match->second.front(),
              std::string_view{}, std::nullopt});
    }

    // Because we can't mutate a map as we are traversing it

    for (const auto &key : to_delete) {
      this->references_.erase(key);
    }

    for (auto &&entry : to_insert) {
      const auto [it, inserted] = this->references_.emplace(std::move(entry));
      set_base_and_fragment(it->second);
    }
  }
}

auto SchemaFrame::locations() const noexcept -> const Locations & {
  return this->locations_;
}

auto SchemaFrame::references() const noexcept -> const References & {
  return this->references_;
}

auto SchemaFrame::reference(const SchemaReferenceType type,
                            const Pointer &pointer) const
    -> std::optional<std::reference_wrapper<const ReferencesEntry>> {
  const auto result{this->references_.find({type, pointer})};
  if (result != this->references_.cend()) {
    return result->second;
  }

  return std::nullopt;
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

auto SchemaFrame::root() const noexcept -> const JSON::String & {
  return this->root_;
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
  return to_uri(this->relative_instance_location(location).concat(
                    relative_schema_location),
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

auto SchemaFrame::traverse(const std::string_view uri) const
    -> std::optional<std::reference_wrapper<const Location>> {
  const JSON::String uri_string{uri};
  const auto static_result{
      this->locations_.find({SchemaReferenceType::Static, uri_string})};
  if (static_result != this->locations_.cend()) {
    return static_result->second;
  }

  const auto dynamic_result{
      this->locations_.find({SchemaReferenceType::Dynamic, uri_string})};
  if (dynamic_result != this->locations_.cend()) {
    return dynamic_result->second;
  }

  return std::nullopt;
}

auto SchemaFrame::traverse(const Pointer &pointer) const
    -> std::optional<std::reference_wrapper<const Location>> {
  // TODO: This is slow. Consider adding a pointer-indexed secondary
  // lookup structure to SchemaFrame
  for (const auto &entry : this->locations_) {
    if (entry.second.pointer == pointer) {
      return entry.second;
    }
  }

  return std::nullopt;
}

auto SchemaFrame::uri(const Pointer &pointer) const
    -> std::optional<std::reference_wrapper<const JSON::String>> {
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

auto SchemaFrame::for_each_resource_uri(
    const std::function<void(std::string_view)> &callback) const -> void {
  for (const auto &[key, location] : this->locations_) {
    if (location.type == LocationType::Resource) {
      callback(key.second);
    }
  }
}

auto SchemaFrame::for_each_unresolved_reference(
    const std::function<void(const Pointer &, const ReferencesEntry &)>
        &callback) const -> void {
  for (const auto &[key, reference] : this->references_) {
    if (!this->traverse(reference.destination).has_value()) {
      callback(key.second, reference);
    }
  }
}

auto SchemaFrame::has_references_to(const Pointer &pointer) const -> bool {
  for (const auto &reference : this->references_) {
    assert(!reference.first.second.empty());
    assert(reference.first.second.back().is_property());

    if (reference.first.first == SchemaReferenceType::Static) {
      const auto match{this->locations_.find(
          {reference.first.first, reference.second.destination})};
      if (match != this->locations_.cend() &&
          match->second.pointer == pointer) {
        return true;
      }
    } else {
      for (const auto &location : this->locations_) {
        if (location.second.type == LocationType::Anchor &&
            location.first.first == SchemaReferenceType::Dynamic &&
            location.second.pointer == pointer) {
          if (!reference.second.fragment.has_value() ||
              URI{location.first.second}.fragment().value_or("") ==
                  reference.second.fragment.value()) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

auto SchemaFrame::has_references_through(const Pointer &pointer) const -> bool {
  for (const auto &reference : this->references_) {
    assert(!reference.first.second.empty());
    assert(reference.first.second.back().is_property());

    if (reference.first.first == SchemaReferenceType::Static) {
      const auto match{this->locations_.find(
          {reference.first.first, reference.second.destination})};
      if (match != this->locations_.cend() &&
          match->second.pointer.starts_with(pointer)) {
        return true;
      }
    } else {
      for (const auto &location : this->locations_) {
        if (location.second.type == LocationType::Anchor &&
            location.first.first == SchemaReferenceType::Dynamic &&
            location.second.pointer.starts_with(pointer)) {
          if (!reference.second.fragment.has_value() ||
              URI{location.first.second}.fragment().value_or("") ==
                  reference.second.fragment.value()) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

auto SchemaFrame::relative_instance_location(const Location &location) const
    -> Pointer {
  return location.pointer.slice(location.relative_pointer);
}

auto SchemaFrame::empty() const noexcept -> bool {
  return this->locations_.empty() && this->references_.empty();
}

auto SchemaFrame::reset() -> void {
  this->root_.clear();
  this->locations_.clear();
  this->references_.clear();
}

} // namespace sourcemeta::core
