class UpgradeDraft3ToDraft4 final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  UpgradeDraft3ToDraft4()
      : SchemaTransformRule{"upgrade_draft_3_to_draft_4"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3) &&
        schema.is_object());

    const bool root_via_default_dialect =
        location.pointer.empty() && !schema.defines("$schema");

    this->stray_required_ =
        has_stray_required_boolean(schema, location.pointer);

    ONLY_CONTINUE_IF(has_pending_draft_3_pattern(schema, location.dialect) ||
                     this->stray_required_ || root_via_default_dialect);

    if (frame.any_subschema_under(
            location.pointer,
            [&root](
                const sourcemeta::core::SchemaFrame::Location &entry) -> bool {
              const auto entry_pointer{
                  sourcemeta::core::to_pointer(entry.pointer)};
              const auto &entry_schema{
                  sourcemeta::core::get(root, entry_pointer)};

              // A Draft 3 spelling beside a `$ref` is dead weight for
              // validation, but Draft 4 does not accept it as a value at all,
              // so it has to be upgraded before the dialect moves rather than
              // being left for the target meta-schema to reject
              return has_pending_draft_3_pattern(entry_schema, entry.dialect) ||
                     has_stray_required_boolean(entry_schema, entry.pointer);
            })) {
      return false;
    }

    return true;
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    for (const auto &[old_pointer, new_pointer] : this->renames_) {
      const auto result{target.rebase(current.concat(old_pointer),
                                      current.concat(new_pointer))};
      if (result != target) {
        return result;
      }
    }

    return target;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    this->renames_.clear();
    rewrite_type_any(schema);
    rewrite_type_array_with_subschemas(schema, this->renames_);
    rewrite_disallow(schema, this->renames_);
    rewrite_extends(schema, this->renames_);
    rewrite_empty_items(schema, this->renames_);
    rewrite_divisible_by(schema);
    // Dropping the stray boolean first leaves the lift free to write its array
    // under the same name. The other way round the lift's array is what gets
    // erased, and the properties it named stop being required
    if (this->stray_required_) {
      schema.erase("required");
    }

    rewrite_required_property_booleans(schema);
    rewrite_dependencies_string_form(schema);
    normalize_dependency_arrays(schema);
    rewrite_format(schema);

    if (schema.defines("$schema") && schema.at("$schema").is_string() &&
        schema.at("$schema").to_string() == DRAFT_3_URL) {
      schema.assign("$schema", sourcemeta::core::JSON{DRAFT_4_URL});
      drop_dialect_overrides(schema, DRAFT_4_URL, this->subschemas());
    } else {
      mark_dialect_override(schema, DRAFT_4_URL);
    }
  }

private:
  using Relocation =
      std::pair<sourcemeta::core::Pointer, sourcemeta::core::Pointer>;

  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DRAFT_3_URL{
      "http://json-schema.org/draft-03/schema#"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DRAFT_4_URL{
      "http://json-schema.org/draft-04/schema#"};

  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::array<std::string_view, 7> PROMOTED_DRAFT_4_KEYWORDS{
      {"multipleOf", "maxProperties", "minProperties", "allOf", "anyOf",
       "oneOf", "not"}};

  mutable bool stray_required_{false};
  mutable std::vector<Relocation> renames_;

  static auto
  has_pending_draft_3_pattern(const sourcemeta::core::JSON &subschema,
                              const std::string_view dialect) -> bool {
    // Either answer putting the subschema past this rung is enough. What it
    // declares covers the marker this rule plants on its own output, without
    // which the keywords it writes would read as pending for good. What
    // framing reads covers a subschema sitting inside a resource that has
    // already moved up, which declares nothing of its own and would otherwise
    // leave the root waiting on work nothing will do
    if (!subschema.is_object() ||
        declares_dialect_out_of_reach(subschema, DRAFT_3_URL) ||
        dialect_position(dialect) > dialect_position(DRAFT_3_URL)) {
      return false;
    }

    if (subschema.defines("$schema") && subschema.at("$schema").is_string() &&
        subschema.at("$schema").to_string() == DRAFT_3_URL) {
      return true;
    }

    const auto *type_value{subschema.try_at("type")};
    if (type_value != nullptr) {
      if (names_every_instance(*type_value)) {
        return true;
      }
      if (type_value->is_array()) {
        for (const auto &element : type_value->as_array()) {
          if (names_every_instance(element)) {
            return true;
          }
          if (element.is_object()) {
            return true;
          }
        }
      }
    }

    const auto *disallow_value{subschema.try_at("disallow")};
    if (disallow_value != nullptr &&
        (disallow_value->is_string() || disallow_value->is_array() ||
         disallow_value->is_object())) {
      return true;
    }

    const auto *extends_value{subschema.try_at("extends")};
    if (extends_value != nullptr &&
        (extends_value->is_array() || extends_value->is_object())) {
      return true;
    }

    if (subschema.defines("divisibleBy")) {
      return true;
    }

    const auto *items{subschema.try_at("items")};
    if (items != nullptr && items->is_array() && items->empty()) {
      return true;
    }

    const auto *properties{subschema.try_at("properties")};
    if (properties != nullptr && properties->is_object()) {
      for (const auto &entry : properties->as_object()) {
        if (entry.second.is_object() && entry.second.defines("required") &&
            entry.second.at("required").is_boolean()) {
          return true;
        }
      }
    }

    const auto *dependencies{subschema.try_at("dependencies")};
    if (dependencies != nullptr && dependencies->is_object()) {
      for (const auto &entry : dependencies->as_object()) {
        if (entry.second.is_string() ||
            (entry.second.is_array() &&
             (entry.second.empty() || has_repeated_name(entry.second)))) {
          return true;
        }
      }
    }

    if (has_renamable_draft_3_format(subschema)) {
      return true;
    }

    // A keyword Draft 4 promotes is inert data here, and the rule that
    // shadows it only fires while the subschema is still read as Draft 3.
    // Bumping an ancestor first takes that reading away, and the keyword
    // starts asserting something the document never said. A descendant this
    // rule has already converted carries the marker that puts it out of
    // reach above, so waiting cannot outlast the work
    return std::ranges::any_of(PROMOTED_DRAFT_4_KEYWORDS,
                               [&subschema](const auto keyword) -> bool {
                                 return subschema.defines(keyword);
                               });
  }

  static auto
  has_renamable_draft_3_format(const sourcemeta::core::JSON &subschema)
      -> bool {
    if (!subschema.is_object()) {
      return false;
    }
    const auto *format_value{subschema.try_at("format")};
    if (format_value == nullptr || !format_value->is_string()) {
      return false;
    }
    const auto &name{format_value->to_string()};
    return name == "host-name" || name == "ip-address";
  }

  // Draft 3 lists the type names it defines and then says that a value outside
  // that list accepts any instance, which is what `any` does. So an
  // unrecognised name is dropped exactly as `any` is, rather than carried into
  // a dialect whose meta-schema accepts only the listed names
  static auto names_every_instance(const sourcemeta::core::JSON &element)
      -> bool {
    if (!element.is_string()) {
      return false;
    }

    static constexpr std::array<std::string_view, 7> DRAFT_3_TYPE_NAMES{
        {"string", "number", "integer", "boolean", "object", "array", "null"}};
    const auto &name{element.to_string()};
    return std::ranges::none_of(
        DRAFT_3_TYPE_NAMES,
        [&name](const auto candidate) -> bool { return candidate == name; });
  }

  static auto rewrite_type_any(sourcemeta::core::JSON &schema) -> void {
    if (!schema.defines("type")) {
      return;
    }
    auto &type_value{schema.at("type")};
    if (type_value.is_string()) {
      if (names_every_instance(type_value)) {
        schema.erase("type");
      }
      return;
    }
    if (!type_value.is_array()) {
      return;
    }
    if (std::ranges::any_of(
            type_value.as_array(),
            [](const auto &element) -> bool { return element.is_object(); })) {
      return;
    }

    if (std::ranges::any_of(type_value.as_array(), names_every_instance)) {
      schema.erase("type");
    }
  }

  static auto
  rewrite_type_array_with_subschemas(sourcemeta::core::JSON &schema,
                                     std::vector<Relocation> &renames) -> void {
    if (!schema.defines("type")) {
      return;
    }
    auto &type_value{schema.at("type")};
    if (!type_value.is_array()) {
      return;
    }
    bool has_subschema{false};
    for (const auto &element : type_value.as_array()) {
      if (element.is_object()) {
        has_subschema = true;
        break;
      }
    }
    if (!has_subschema) {
      return;
    }

    auto branches{sourcemeta::core::JSON::make_array()};
    for (const auto &element : type_value.as_array()) {
      if (element.is_string()) {
        branches.push_back(type_name_to_branch(element));
      } else if (element.is_object()) {
        branches.push_back(element);
      }
    }
    schema.erase("type");
    schema.assign("anyOf", std::move(branches));
    renames.emplace_back(sourcemeta::core::Pointer{"type"},
                         sourcemeta::core::Pointer{"anyOf"});
  }

  // A name matching every instance becomes a schema that accepts anything,
  // since no later dialect defines that name as a type
  static auto type_name_to_branch(const sourcemeta::core::JSON &type_name)
      -> sourcemeta::core::JSON {
    auto branch{sourcemeta::core::JSON::make_object()};
    if (!names_every_instance(type_name)) {
      branch.assign("type", type_name);
    }

    return branch;
  }

  static auto rewrite_disallow(sourcemeta::core::JSON &schema,
                               std::vector<Relocation> &renames) -> void {
    if (!schema.defines("disallow") || schema.defines("not")) {
      return;
    }

    const auto &disallow{schema.at("disallow")};
    if (!disallow.is_string() && !disallow.is_array() &&
        !disallow.is_object()) {
      return;
    }

    if (disallow.is_string() && names_every_instance(disallow)) {
      schema.erase("disallow");
      schema.assign("not", sourcemeta::core::JSON::make_object());
      return;
    }

    // A name matching every instance makes the whole union match everything,
    // so the result rejects everything whatever else is listed. Collapsing
    // straight to that answer is only safe while the other entries are type
    // names: a schema among them is a schema a reference can name, and
    // dropping it would leave that reference pointing nowhere. Carrying every
    // entry over as a branch says the same thing and keeps them all addressable
    if (disallow.is_array() &&
        std::ranges::any_of(disallow.as_array(), names_every_instance) &&
        std::ranges::none_of(
            disallow.as_array(),
            [](const auto &element) -> bool { return element.is_object(); })) {
      schema.erase("disallow");
      schema.assign("not", sourcemeta::core::JSON::make_object());
      return;
    }

    auto negated{sourcemeta::core::JSON::make_object()};
    if (disallow.is_string()) {
      negated.assign("type", disallow);
    } else if (disallow.is_array()) {
      bool has_subschema{false};
      for (const auto &element : disallow.as_array()) {
        if (element.is_object()) {
          has_subschema = true;
          break;
        }
      }
      if (!has_subschema) {
        negated.assign("type", disallow);
      } else {
        auto branches{sourcemeta::core::JSON::make_array()};
        for (const auto &element : disallow.as_array()) {
          if (element.is_string()) {
            branches.push_back(type_name_to_branch(element));
          } else if (element.is_object()) {
            branches.push_back(element);
          }
        }
        negated.assign("anyOf", std::move(branches));

        // A reference may name a schema that sat in here, so where these
        // land is recorded for `rereference` to follow. Each branch records
        // its own move, as an author's `disallow` schema may define `anyOf`
        // itself and reading the result back cannot tell the two apart
        renames.emplace_back(sourcemeta::core::Pointer{"disallow"},
                             sourcemeta::core::Pointer{"not", "anyOf"});
      }
    } else {
      negated = disallow;
      renames.emplace_back(sourcemeta::core::Pointer{"disallow"},
                           sourcemeta::core::Pointer{"not"});
    }

    // The wrapper is a subschema this rule just wrote, and the keywords it
    // holds are Draft 4 spellings rather than the author's data. Saying so
    // keeps the rule that shadows a promoted keyword from reading them as
    // something inert that has to be moved out of the way
    mark_dialect_override(negated, DRAFT_4_URL);

    schema.erase("disallow");
    schema.assign("not", std::move(negated));
  }

  static auto rewrite_extends(sourcemeta::core::JSON &schema,
                              std::vector<Relocation> &renames) -> void {
    if (!schema.defines("extends") || schema.defines("allOf")) {
      return;
    }

    const auto &extends{schema.at("extends")};
    if (!extends.is_array() && !extends.is_object()) {
      return;
    }

    auto value{extends};
    schema.erase("extends");

    if (value.is_array()) {
      // Draft 3 satisfies an empty `extends` vacuously, while Draft 4 asks
      // `allOf` for at least one element, so there is nothing to carry over
      if (value.empty()) {
        return;
      }

      renames.emplace_back(sourcemeta::core::Pointer{"extends"},
                           sourcemeta::core::Pointer{"allOf"});
      schema.assign("allOf", std::move(value));
      return;
    }

    auto array{sourcemeta::core::JSON::make_array()};
    array.push_back(std::move(value));
    renames.emplace_back(sourcemeta::core::Pointer{"extends"},
                         sourcemeta::core::Pointer{"allOf", 0});
    schema.assign("allOf", std::move(array));
  }

  // Draft 3 takes an empty `items` array as naming no position at all, so
  // every element falls to `additionalItems`. Draft 4 asks a schema array for
  // at least one entry, so the empty one cannot come along. Saying the same
  // thing there means letting the `additionalItems` schema apply to every
  // element, which is what a single-schema `items` does. Dropping the empty
  // array alone would instead leave `additionalItems` with no array beside it,
  // where both dialects ignore it, and every element would stop being checked
  static auto rewrite_empty_items(sourcemeta::core::JSON &schema,
                                  std::vector<Relocation> &renames) -> void {
    if (!schema.defines("items") || !schema.at("items").is_array() ||
        !schema.at("items").empty()) {
      return;
    }

    if (!schema.defines("additionalItems")) {
      schema.erase("items");
      return;
    }

    // Draft 3 lets this keyword be a boolean, which Draft 4 does not accept
    // where it is going. `true` allows every element, which is what saying
    // nothing does, and `false` allows none, which is an array that has to be
    // empty. `maxItems` says that in a keyword both dialects share, so no
    // keyword Draft 4 only just promoted is introduced here
    if (schema.at("additionalItems").is_boolean()) {
      const auto allows{schema.at("additionalItems").to_boolean()};
      schema.erase("additionalItems");
      schema.erase("items");
      if (!allows) {
        schema.assign("maxItems", sourcemeta::core::JSON{0});
      }

      return;
    }

    // A reference may name the schema being moved, or something inside it, so
    // where it lands is recorded for `rereference` to follow
    renames.emplace_back(sourcemeta::core::Pointer{"additionalItems"},
                         sourcemeta::core::Pointer{"items"});
    schema.assign("items", schema.at("additionalItems"));
    schema.erase("additionalItems");
  }

  static auto rewrite_divisible_by(sourcemeta::core::JSON &schema) -> void {
    if (!schema.defines("divisibleBy") || schema.defines("multipleOf")) {
      return;
    }
    schema.rename("divisibleBy", "multipleOf");
  }

  // A boolean `required` is a Draft 3 spelling, so it is only this rule's to
  // move when the property is read as Draft 3 as well. One that names a dialect
  // of its own answers to that dialect instead, where the member may be nothing
  // but author data, and lifting it would destroy it there while inventing an
  // assertion here that the document never made
  static auto reads_as_draft_3(const sourcemeta::core::JSON &property) -> bool {
    const auto declared{declared_dialect(property)};
    return declared.empty() ||
           dialect_position(declared) == dialect_position(DRAFT_3_URL);
  }

  static auto rewrite_required_property_booleans(sourcemeta::core::JSON &schema)
      -> void {
    if (!schema.defines("properties") || !schema.at("properties").is_object()) {
      return;
    }

    std::vector<std::string> newly_required;
    auto &properties{schema.at("properties")};
    std::vector<std::string> property_keys;
    property_keys.reserve(properties.size());
    for (const auto &entry : properties.as_object()) {
      property_keys.push_back(entry.first);
    }

    for (const auto &key : property_keys) {
      auto &property{properties.at(key)};
      if (!property.is_object() || !property.defines("required")) {
        continue;
      }

      if (!reads_as_draft_3(property)) {
        continue;
      }
      const auto &required_value{property.at("required")};
      if (!required_value.is_boolean()) {
        continue;
      }
      // Draft 3 replaces a schema with whatever its `$ref` names, so a
      // `required` flag beside one never made the property mandatory. Lifting
      // it onto the parent, where nothing suppresses it, would invent an
      // assertion the document never made, and erasing it would throw away the
      // only record that the author asked for something the dialect ignored.
      // Shadowing keeps that record without asserting anything, which is how
      // every other suppressed sibling is carried over. A `false` flag is the
      // default and says nothing, so it goes
      if (property.defines("$ref")) {
        if (required_value.to_boolean()) {
          std::string shadowed{"x-required"};
          while (property.defines(shadowed)) {
            shadowed.insert(0, "x-");
          }

          property.rename("required", std::move(shadowed));
        } else {
          property.erase("required");
        }

        continue;
      }

      const bool is_required{required_value.to_boolean()};
      property.erase("required");
      if (is_required) {
        newly_required.push_back(key);
      }
    }

    if (newly_required.empty()) {
      return;
    }

    if (!schema.defines("required") || !schema.at("required").is_array()) {
      auto fresh{sourcemeta::core::JSON::make_array()};
      for (const auto &name : newly_required) {
        fresh.push_back(sourcemeta::core::JSON{name});
      }
      schema.try_assign_before("required", fresh, "properties");
      return;
    }

    auto &existing{schema.at("required")};
    std::set<std::string> already;
    for (const auto &item : existing.as_array()) {
      if (item.is_string()) {
        already.insert(item.to_string());
      }
    }
    for (const auto &name : newly_required) {
      if (already.contains(name)) {
        continue;
      }
      existing.push_back(sourcemeta::core::JSON{name});
      already.insert(name);
    }
  }

  // Draft 3 reads this keyword as whether the instance it sits on has to be
  // present, which is a question only the enclosing `properties` can answer.
  // One directly under `properties` is that enclosing schema's to lift, so
  // only one anywhere else is stray
  static auto
  has_stray_required_boolean(const sourcemeta::core::JSON &subschema,
                             const sourcemeta::core::WeakPointer &pointer)
      -> bool {
    if (!subschema.is_object() ||
        declares_dialect_out_of_reach(subschema, DRAFT_3_URL)) {
      return false;
    }

    const auto *required{subschema.try_at("required")};
    if (required == nullptr || !required->is_boolean()) {
      return false;
    }

    return pointer.size() < 2 ||
           !pointer.at(pointer.size() - 2).is_property() ||
           pointer.at(pointer.size() - 2).to_property() != "properties";
  }

  static auto has_repeated_name(const sourcemeta::core::JSON &names) -> bool {
    std::set<std::string> already;
    for (const auto &name : names.as_array()) {
      if (name.is_string() && !already.insert(name.to_string()).second) {
        return true;
      }
    }

    return false;
  }

  // Draft 3 satisfies an empty property dependency vacuously and asks for the
  // same property twice when a name repeats, while Draft 4 asks such an array
  // for at least one element and for its elements to be unique
  static auto normalize_dependency_arrays(sourcemeta::core::JSON &schema)
      -> void {
    if (!schema.defines("dependencies") ||
        !schema.at("dependencies").is_object()) {
      return;
    }

    auto &dependencies{schema.at("dependencies")};
    std::vector<std::string> vacuous_keys;
    std::vector<std::string> repeating_keys;
    for (const auto &entry : dependencies.as_object()) {
      if (!entry.second.is_array()) {
        continue;
      }

      if (entry.second.empty()) {
        vacuous_keys.push_back(entry.first);
      } else if (has_repeated_name(entry.second)) {
        repeating_keys.push_back(entry.first);
      }
    }

    for (const auto &key : repeating_keys) {
      auto fresh{sourcemeta::core::JSON::make_array()};
      std::set<std::string> already;
      for (const auto &name : dependencies.at(key).as_array()) {
        if (name.is_string() && !already.insert(name.to_string()).second) {
          continue;
        }

        fresh.push_back(name);
      }

      dependencies.assign(key, std::move(fresh));
    }

    for (const auto &key : vacuous_keys) {
      dependencies.erase(key);
    }

    if (dependencies.empty()) {
      schema.erase("dependencies");
    }
  }

  static auto rewrite_dependencies_string_form(sourcemeta::core::JSON &schema)
      -> void {
    if (!schema.defines("dependencies") ||
        !schema.at("dependencies").is_object()) {
      return;
    }
    auto &dependencies{schema.at("dependencies")};
    std::vector<std::string> string_keys;
    for (const auto &entry : dependencies.as_object()) {
      if (entry.second.is_string()) {
        string_keys.push_back(entry.first);
      }
    }
    for (const auto &key : string_keys) {
      const auto value{dependencies.at(key).to_string()};
      auto array{sourcemeta::core::JSON::make_array()};
      array.push_back(sourcemeta::core::JSON{value});
      dependencies.assign(key, std::move(array));
    }
  }

  static auto rewrite_format(sourcemeta::core::JSON &schema) -> void {
    if (!schema.defines("format")) {
      return;
    }
    auto &format_value{schema.at("format")};
    if (!format_value.is_string()) {
      return;
    }
    const auto &name{format_value.to_string()};
    if (name == "host-name") {
      schema.assign("format", sourcemeta::core::JSON{"hostname"});
    } else if (name == "ip-address") {
      schema.assign("format", sourcemeta::core::JSON{"ipv4"});
    }
  }
};
