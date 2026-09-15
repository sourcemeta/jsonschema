class UnevaluatedPropertiesToAdditionalProperties final
    : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  UnevaluatedPropertiesToAdditionalProperties()
      : SchemaTransformRule{"unevaluated_properties_to_additional_properties"} {
        };

  [[nodiscard]] auto condition(
      const sourcemeta::core::JSON &schema, const sourcemeta::core::JSON &root,
      const sourcemeta::core::SchemaVocabularies &vocabularies,
      const sourcemeta::core::SchemaFrame &frame,
      const sourcemeta::core::SchemaFrame::Location &location,
      const sourcemeta::core::SchemaWalker &walker,
      const sourcemeta::core::SchemaResolver &resolver) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_UNEVALUATED,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR}) &&
        schema.is_object() && schema.defines("unevaluatedProperties"));

    // We are going to write into `additionalProperties`, so a sibling of that
    // name has to keep us away. A sibling `unevaluatedItems` does not: it
    // consumes the annotations that decide which *items* are evaluated, and
    // neither merging property names into `properties` nor renaming
    // `unevaluatedProperties` changes any of those. Each of the two
    // annotation-dependent keywords is eliminated against its own evaluated
    // set, so converting one of them here leaves the other meaning what it
    // always meant
    ONLY_CONTINUE_IF(!schema.defines("additionalProperties"));

    // The only in-place applicator we know how to statically account for is an
    // `allOf` whose every branch consists of nothing but one `$ref`. Because
    // `allOf` applies each branch and demands that all of them succeed, the
    // property set it evaluates is exactly the union of what its targets
    // spell out, with no intersection to compute. A branch of any other shape
    // could contribute annotations we cannot enumerate, and rather than
    // classify which of those shapes happen to be harmless we decline the
    // parent outright and convert nothing.
    //
    // The keyword is optional. A schema carrying no `allOf` at all evaluates
    // whatever its own `properties` and `patternProperties` spell out and
    // nothing more, which is the degenerate case of the same reasoning: an
    // empty union to merge, leaving the rename on its own
    const auto *all_of{schema.try_at("allOf")};
    if (all_of != nullptr) {
      ONLY_CONTINUE_IF(all_of->is_array());
      for (const auto &branch : all_of->as_array()) {
        if (!branch.is_object() || branch.size() != 1) {
          return false;
        }
        const auto *reference{branch.try_at("$ref")};
        if (reference == nullptr || !reference->is_string()) {
          return false;
        }
      }
    }

    // Any other in-place applicator could contribute property annotations that
    // we cannot enumerate at compile time, and `properties` along with
    // `patternProperties` are the only other keywords that decide what counts
    // as evaluated at this level. Everything else is irrelevant to the rewrite
    for (const auto &entry : schema.as_object()) {
      if (entry.first == "unevaluatedProperties" || entry.first == "allOf" ||
          entry.first == "properties" || entry.first == "patternProperties") {
        continue;
      }
      const auto keyword_type{walker(entry.first, vocabularies).type};
      if (is_in_place_applicator(keyword_type) ||
          keyword_type == sourcemeta::core::SchemaKeywordType::Reference) {
        return false;
      }
    }

    const auto *properties{schema.try_at("properties")};
    ONLY_CONTINUE_IF(!properties || properties->is_object());

    this->properties_.clear();
    if (all_of == nullptr || all_of->empty()) {
      return true;
    }

    // Resolve each branch through the frame rather than through the raw
    // reference string, so that we honour whatever base the reference
    // was written against. A branch whose reference the frame never hands
    // us leaves its slot empty, which declines the parent below
    std::vector<const sourcemeta::core::JSON::String *> destinations(
        all_of->size(), nullptr);
    frame.for_each_reference_from(
        location.pointer,
        [&destinations, &location](
            const sourcemeta::core::SchemaReferenceType type,
            const sourcemeta::core::WeakPointer &source,
            const sourcemeta::core::SchemaFrame::Reference &entry_ref) -> void {
          if (type != sourcemeta::core::SchemaReferenceType::Static) {
            return;
          }
          const auto relative{source.resolve_from(location.pointer)};
          if (relative.size() == 3 && relative.at(0).is_property() &&
              relative.at(0).to_property() == "allOf" &&
              relative.at(1).is_index() &&
              relative.at(1).to_index() < destinations.size() &&
              relative.at(2).is_property() &&
              relative.at(2).to_property() == "$ref") {
            auto *&slot{destinations.at(relative.at(1).to_index())};
            if (slot == nullptr) {
              slot = &entry_ref.destination;
            }
          }
        });

    // Gather into a local rather than straight into the member, so that a
    // target failing halfway through cannot leave the rule holding the names
    // of the targets that came before it
    std::vector<sourcemeta::core::JSON::String> collected;
    for (const auto *destination : destinations) {
      if (destination == nullptr) {
        return false;
      }

      if (!collect_target_properties(root, frame, walker, resolver,
                                     *destination, properties, collected)) {
        return false;
      }
    }

    this->properties_ = std::move(collected);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (!this->properties_.empty()) {
      schema.assign_if_missing("properties",
                               sourcemeta::core::JSON::make_object());
      for (const auto &property : this->properties_) {
        schema.at("properties").assign(property, sourcemeta::core::JSON{true});
      }
    }

    schema.rename("unevaluatedProperties", "additionalProperties");
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    return target.rebase(current.concat("unevaluatedProperties"),
                         current.concat("additionalProperties"));
  }

private:
  // Whether the schema a branch points to evaluates a property set we can name
  // upfront, appending the names it contributes when it does. Every target is
  // judged on its own and against the same conditions, so that one target
  // being opaque declines the whole parent rather than converting the branches
  // that did pass
  [[nodiscard]] static auto collect_target_properties(
      const sourcemeta::core::JSON &root,
      const sourcemeta::core::SchemaFrame &frame,
      const sourcemeta::core::SchemaWalker &walker,
      const sourcemeta::core::SchemaResolver &resolver,
      const sourcemeta::core::JSON::String &destination,
      const sourcemeta::core::JSON *const properties,
      std::vector<sourcemeta::core::JSON::String> &collected) -> bool {
    const auto target{frame.traverse(destination)};
    ONLY_CONTINUE_IF(target.has_value());
    const auto &target_location{target.value().get()};
    const auto &target_schema{
        sourcemeta::core::get(root, target_location.pointer)};
    ONLY_CONTINUE_IF(target_schema.is_object());

    // The referenced schema must evaluate a property set that we can name
    // upfront, which means no further indirection of its own and no keyword
    // that evaluates properties it does not spell out one by one. Whatever the
    // target says about items is none of our business, for the same reason a
    // sibling `unevaluatedItems` on the parent is not
    ONLY_CONTINUE_IF(!target_schema.defines("unevaluatedProperties") &&
                     !target_schema.defines("additionalProperties"));
    const auto *target_pattern_properties{
        target_schema.try_at("patternProperties")};
    ONLY_CONTINUE_IF(!target_pattern_properties ||
                     (target_pattern_properties->is_object() &&
                      target_pattern_properties->empty()));

    // Older dialects grow an implicit `additionalProperties` during
    // canonicalisation, which would silently widen the property set we are
    // about to freeze, so we only reason about the modern ones
    const auto &target_vocabularies{
        frame.vocabularies(target_location, resolver)};
    ONLY_CONTINUE_IF(target_vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
         SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR}));

    for (const auto &entry : target_schema.as_object()) {
      const auto keyword_type{walker(entry.first, target_vocabularies).type};
      if (is_in_place_applicator(keyword_type) ||
          keyword_type == sourcemeta::core::SchemaKeywordType::Reference) {
        return false;
      }
    }

    const auto *target_properties{target_schema.try_at("properties")};
    ONLY_CONTINUE_IF(!target_properties || target_properties->is_object());

    // Only trust a target whose `required` its `properties` already spells
    // out. This is a point-in-time check, not a guarantee: nothing stops
    // `required_properties_in_properties` from filling in the missing entry
    // on an earlier pass, in which case the target really does evaluate that
    // name by the time we look and merging it is faithful to what we see. The
    // check simply keeps us from being the rule that first depends on a name
    // whose evaluation status is still in flux
    const auto *target_required{target_schema.try_at("required")};
    if (target_required != nullptr && target_required->is_array()) {
      for (const auto &entry : target_required->as_array()) {
        if (entry.is_string() &&
            !(target_properties != nullptr &&
              target_properties->defines(entry.to_string()))) {
          return false;
        }
      }
    }

    if (target_properties != nullptr) {
      for (const auto &entry : target_properties->as_object()) {
        if (properties != nullptr && properties->defines(entry.first)) {
          continue;
        }

        // Targets may well spell out the same name, and the union counts it
        // once
        if (std::ranges::none_of(collected, [&entry](const auto &name) -> bool {
              return name == entry.first;
            })) {
          collected.emplace_back(entry.first);
        }
      }
    }

    return true;
  }

  mutable std::vector<sourcemeta::core::JSON::String> properties_;
};
