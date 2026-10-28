class UpgradeOpenAPI30ToDraft4 final : public SchemaTransformRule {
public:
  UpgradeOpenAPI30ToDraft4(const ConvertTarget target)
      : SchemaTransformRule{"upgrade_openapi_3_0_to_draft_4"},
        keeps_read_only_{names_read_only(target)},
        keeps_deprecated_{names_deprecated(target)},
        keeps_openapi_annotations_{names_openapi_annotations(target)},
        keeps_default_mapping_{target == ConvertTarget::OpenAPI32} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &site, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(SchemaVocabularies::Known::OPENAPI_3_0_BASE) &&
        schema.is_object());

    return holds_openapi_3_0_override(schema) ||
           holds_ignored_sibling(schema) ||
           has_pending_openapi_3_0_pattern(schema) ||
           holds_promoted_keyword(schema) ||
           holds_movable_definitions(schema) || holds_pending_shape(schema) ||
           at_dialect_declaration(schema, site);
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &site) const
      -> void override {
    this->renames_.clear();
    this->relocations_.clear();

    // OpenAPI 3.0 section 4.7.23: a Reference Object cannot be extended with
    // additional properties, and any property added to one SHALL be ignored. An
    // object carrying `$ref` can only be a Reference Object, because the Schema
    // Object closes itself to additional properties and does not name `$ref`,
    // so every member beside it was ignored and has to go on being ignored.
    //
    // Carrying one across as the keyword a target names does two wrong things
    // at once: it reads a constraint the source never applied, and it hands the
    // target's own meta-schema a value to type-check that nothing ever checked.
    // So none of the rewriting below applies to a Reference Object, and its
    // members go across as the author data they already were
    if (schema.defines("$ref")) {
      for (const auto &name : member_names(schema)) {
        if (name == "$ref" || name == "$schema" || name.starts_with("x-")) {
          continue;
        }

        this->shadow(schema, name);
      }
    } else {
      // Before the shadowing below, so that a `nullable` this consumes is gone
      // by the time the annotations are renamed and one it declines is still
      // there to be renamed with them
      fold_nullable_into_type(schema);

      // Comparing the two meta-schemas member by member, the OpenAPI 3.0 Schema
      // Object is looser in exactly three places and equal or stricter
      // everywhere else: `enum` states `uniqueItems: false`, the three
      // applicator arrays carry no `minItems`, and it states none of the
      // keyword dependencies Draft 4 does. The first two are shapes, rewritten
      // here, and the third is answered for below
      deduplicate_enum(schema);
      fold_empty_applicators(schema);

      // Draft 4 requires each of these to accompany the bound it modifies,
      // while the OpenAPI 3.0 Schema Object states no dependency between
      // keywords at all. One standing on its own modifies a bound that is not
      // there, and an `allOf` with no branches asserts nothing of the instance,
      // so each goes across as the author data it was. None of the three can
      // stay where it is: Draft 4 reads an incomplete bound and an empty
      // applicator as keywords it will not accept
      for (const auto &[keyword, companion] : UNACCOMPANIED) {
        if (schema.defines(sourcemeta::core::JSON::String{keyword}) &&
            !schema.defines(sourcemeta::core::JSON::String{companion})) {
          this->shadow(schema, keyword);
        }
      }

      // After the fold above, which puts what an empty `anyOf` or `oneOf` means
      // into `allOf` and so reuses an empty one as the container it needs. An
      // `allOf` still empty here is one the author wrote alone
      if (schema.defines("allOf") && schema.at("allOf").is_array() &&
          schema.at("allOf").empty()) {
        this->shadow(schema, "allOf");
      }

      for (const auto &keyword : KEYWORDS) {
        if (!schema.defines(sourcemeta::core::JSON::String{keyword})) {
          continue;
        }

        if (this->carries_as_keyword(schema, keyword)) {
          this->carry(schema, keyword);
        } else {
          this->shadow(schema, keyword);
        }
      }

      // Taken out before the shadowing below, so that what moves is the map the
      // author wrote under this name and not a `definitions` that the shadowing
      // has since put there. Doing it the other way round hands the shadowed
      // member straight back to the move, which promotes the author's data to
      // the keyword the shadowing was there to protect it from
      auto definitions{take_definitions(schema, this->relocations_)};

      for (const auto &keyword : PROMOTED_BY_DRAFT_4) {
        this->shadow(schema, keyword);
      }

      if (!definitions.empty()) {
        schema.assign("definitions", std::move(definitions));
      }
    }

    bump_dialect(schema, site, DRAFT_4_URL);
  }

  // A reference is rebased through these in order, so one whose source names a
  // position another's destination sits under has to come first. Shadowing
  // `definitions` names the very position the move below places its entries
  // under, so the renames are published ahead of the moves: the other way
  // round, the rename catches an entry the move has just put there and sends
  // every reference to it somewhere nothing lives
  [[nodiscard]] auto relocations() const -> std::vector<Relocation> override {
    std::vector<Relocation> result;
    result.reserve(this->renames_.size() + this->relocations_.size());
    for (const auto &[old_name, new_name] : this->renames_) {
      result.emplace_back(sourcemeta::core::Pointer{old_name},
                          sourcemeta::core::Pointer{new_name});
    }

    result.insert(result.cend(), this->relocations_.cbegin(),
                  this->relocations_.cend());
    return result;
  }

private:
  // The only member the dialect reads a named subschema under, which is how a
  // document on it keeps anything reusable at all. Its applicators hold
  // subschemas too, but none of them by a name the author chooses
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DEFINITIONS{"x-definitions"};

  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DRAFT_4_URL{
      "http://json-schema.org/draft-04/schema#"};

  // What the OpenAPI 3.0 Schema Object adds that Draft 4 has no word for, less
  // the `nullable` the rewrite above consumes. Every one of these annotates
  // rather than asserts, so shadowing one changes no verdict, and Draft 4 reads
  // an `x-` member as the unknown keyword it is. All but `nullable` are
  // shadowed only where the target has no word for them, which
  // `carries_as_keyword` settles: three became keywords of a later draft, and
  // four are named by the OpenAPI dialects above 3.0
  // Which targets name each of the keywords this edge can carry across. Spelled
  // out rather than compared, because `ConvertTarget` is a public enumeration
  // that promises no ordering, so reading one out of the declaration order
  // would make rearranging that header change what this rule does
  static auto names_read_only(const ConvertTarget target) -> bool {
    return target == ConvertTarget::Draft7 ||
           target == ConvertTarget::Draft201909 ||
           target == ConvertTarget::Draft202012 ||
           target == ConvertTarget::OpenAPI31 ||
           target == ConvertTarget::OpenAPI32;
  }

  static auto names_deprecated(const ConvertTarget target) -> bool {
    return target == ConvertTarget::Draft201909 ||
           target == ConvertTarget::Draft202012 ||
           target == ConvertTarget::OpenAPI31 ||
           target == ConvertTarget::OpenAPI32;
  }

  static auto names_openapi_annotations(const ConvertTarget target) -> bool {
    return target == ConvertTarget::OpenAPI31 ||
           target == ConvertTarget::OpenAPI32;
  }

  static constexpr std::array<std::string_view, 8> KEYWORDS{
      {"nullable", "discriminator", "xml", "externalDocs", "example",
       "deprecated", "readOnly", "writeOnly"}};

  // Where a `mapping` entry names a schema inside the container this edge moves
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string MAPPING_INTO_DEFINITIONS{"#/x-definitions/"};

  // Each of these modifies the bound beside it, which Draft 4 requires to be
  // there and OpenAPI 3.0 does not
  static constexpr std::array<std::pair<std::string_view, std::string_view>, 2>
      UNACCOMPANIED{
          {{"exclusiveMinimum", "minimum"}, {"exclusiveMaximum", "maximum"}}};

  // The other direction: what Draft 4 names that the OpenAPI 3.0 Schema Object
  // does not, so a document carrying one carries author data that the target
  // would start reading as a keyword. `id` is the one that matters most, as
  // Draft 4 takes it for the identifier that every reference in the document
  // resolves against.
  //
  // Two positions can carry one of these, and the Schema Object is neither:
  // there the dialect prohibits all five outright. A Reference Object
  // constrains nothing beyond its `$ref`, though the branch above answers for
  // that one by shadowing every member it holds. The other is an entry of an
  // `x-definitions` map, which the dialect constrains with `^x-` and so not at
  // all. That is the position this list is for, and it is narrow: an entry
  // something references is read as a Schema Object and refused, so what
  // survives to here sits in an entry nothing reads. It still has to be
  // shadowed, because the member would otherwise reach the target as a keyword
  // whose value makes the output invalid against the dialect it declares.
  //
  // `$schema` is left out because it is the one of the six that names a
  // dialect. Where framing would read one, `bump_dialect` writes the rung's own
  // in its place, and anywhere else `ShadowStrayDialectDeclaration` carries it
  // across as the author data it is
  static constexpr std::array<std::string_view, 5> PROMOTED_BY_DRAFT_4{
      {"id", "definitions", "dependencies", "patternProperties",
       "additionalItems"}};

  // Carry a member across as the author data it is, under a name the target
  // reads as nothing. A name already taken gains another prefix rather than
  // overwriting what is there
  auto shadow(sourcemeta::core::JSON &schema,
              const std::string_view keyword) const -> void {
    const std::string keyword_name{keyword};
    if (!schema.defines(keyword_name)) {
      return;
    }

    std::string prefixed_name{"x-" + keyword_name};
    while (schema.defines(prefixed_name) || names_core_reads(prefixed_name)) {
      prefixed_name.insert(0, "x-");
    }

    this->renames_.emplace(keyword_name, prefixed_name);
    schema.rename(keyword_name, std::move(prefixed_name));
  }

  // Park an annotation under the name the last phase reads it back from.
  //
  // Handing it to the ladder under its own name would ask every rung that
  // promotes that name to recognise this one and stand down, and no rung can:
  // by the time one runs, the dialect this edge moved off is gone from the
  // document, and a position this edge rewrote reads exactly like one written
  // in the dialect it now declares. Under a name no dialect reads, the
  // annotation climbs as the author data it looks like and no rung has to be
  // told anything at all.
  //
  // The name is free wherever this runs, because a member the author wrote
  // under it is moved aside before any rung is planned
  auto carry(sourcemeta::core::JSON &schema,
             const std::string_view keyword) const -> void {
    const sourcemeta::core::JSON::String keyword_name{keyword};
    auto parked{carried_name(keyword)};
    this->renames_.emplace(keyword_name, parked);
    schema.rename(keyword_name, std::move(parked));
  }

  // Whether this subschema still carries something of the dialect that the
  // transform will act on. Each of those is consumed, shadowed or parked, so
  // the answer turns false once it has run and the conversion stops finding
  // work here
  static auto
  has_pending_openapi_3_0_pattern(const sourcemeta::core::JSON &subschema)
      -> bool {
    for (const auto &keyword : KEYWORDS) {
      if (subschema.defines(sourcemeta::core::JSON::String{keyword})) {
        return true;
      }
    }

    return false;
  }

  // Three of these became keywords of a later draft with the meaning OpenAPI
  // 3.0 already gave them, so a target defining one takes it as it stands
  // rather than as the author data the shadowing would make of it. `readOnly`
  // and `writeOnly` are native from Draft 7 and `deprecated` from 2019-09.
  //
  // This is the one place in the module where a rule reads the target the
  // caller finally asked for rather than the rung in front of it. It has to:
  // Draft 4 cannot express any of the three, so what the waypoint should carry
  // is only answerable from where the document is going. The cost is that
  // converting by way of Draft 4 no longer reproduces the direct result for
  // these three, which `convertsuite` records per fixture.
  //
  // The value answers as well as the target. This dialect constrained each of
  // these, so a member carrying a shape it never allowed was never the keyword
  // it is named after, and handing that to the target would leave an output
  // the target's own meta-schema rejects. It goes across as the author data it
  // has to be instead
  [[nodiscard]] auto carries_as_keyword(const sourcemeta::core::JSON &subschema,
                                        const std::string_view keyword) const
      -> bool {
    const auto &value{subschema.at(sourcemeta::core::JSON::String{keyword})};

    if (keyword == "readOnly" || keyword == "writeOnly") {
      return this->keeps_read_only_ && value.is_boolean();
    }

    if (keyword == "deprecated") {
      return this->keeps_deprecated_ && value.is_boolean();
    }

    if (!this->keeps_openapi_annotations_) {
      return false;
    }

    // Of the four the OpenAPI dialects name for themselves, only this one is
    // stated more loosely by 3.0 than by the dialects above it, so only this
    // one has more than a shape to answer for
    if (keyword == "discriminator") {
      return this->is_expressible_discriminator(value) &&
             mapping_survives_the_move(value);
    }

    if (keyword == "xml" || keyword == "externalDocs") {
      return value.is_object();
    }

    // `nullable` is not among them: no OpenAPI dialect above 3.0 has it, as a
    // type union says it there instead. `example` names anything at all, so
    // there is no shape of it to refuse
    return keyword == "example";
  }

  // A `mapping` value may name a schema by reference, and the frame indexes no
  // such reference, so nothing follows one when the schema it names moves. The
  // one member this edge moves is the subschema container, so a mapping naming
  // a position inside it would be left pointing where nothing is any more.
  // Carrying that across as a keyword would publish a discriminator that lies
  // about where its branches are, so it goes across as author data instead,
  // which at least reads as the record of the author's intent that it is
  static auto
  mapping_survives_the_move(const sourcemeta::core::JSON &discriminator)
      -> bool {
    const auto *mapping{discriminator.try_at("mapping")};
    if (mapping == nullptr || !mapping->is_object()) {
      return true;
    }

    for (const auto &entry : mapping->as_object()) {
      if (entry.second.is_string() &&
          entry.second.to_string().starts_with(MAPPING_INTO_DEFINITIONS)) {
        return false;
      }
    }

    return true;
  }

  // OpenAPI 3.0 leaves its Discriminator Object open, naming `propertyName` and
  // `mapping` without closing the object to anything else, while 3.1 and 3.2
  // close theirs to those two and the extension prefix. One carrying anything
  // more is a discriminator the target cannot read, so it goes across as the
  // author data it has to be there rather than as a keyword that would make the
  // output invalid against the dialect it declares.
  //
  // Which members are asked about depends on the target, because the two
  // shapes differ: 3.1 names `propertyName` and `mapping`, and 3.2 names
  // `defaultMapping` as well. A discriminator carrying that one is a
  // discriminator 3.2 reads and 3.1 does not
  [[nodiscard]] auto is_expressible_discriminator(
      const sourcemeta::core::JSON &discriminator) const -> bool {
    if (!discriminator.is_object()) {
      return false;
    }

    for (const auto &entry : discriminator.as_object()) {
      if (entry.first == "propertyName" || entry.first == "mapping" ||
          entry.first.starts_with("x-")) {
        continue;
      }

      if (entry.first == "defaultMapping" && this->keeps_default_mapping_) {
        continue;
      }

      return false;
    }

    return true;
  }

  static auto holds_promoted_keyword(const sourcemeta::core::JSON &schema)
      -> bool {
    for (const auto &keyword : PROMOTED_BY_DRAFT_4) {
      if (schema.defines(sourcemeta::core::JSON::String{keyword})) {
        return true;
      }
    }

    return false;
  }

  // The members of a Reference Object that the dialect ignored and that a
  // target would start reading. One already carrying the extension prefix is
  // read as nothing anywhere, and a `$schema` is answered for by the rule that
  // settles a dialect declaration
  static auto holds_ignored_sibling(const sourcemeta::core::JSON &schema)
      -> bool {
    if (!schema.defines("$ref")) {
      return false;
    }

    for (const auto &entry : schema.as_object()) {
      if (entry.first != "$ref" && entry.first != "$schema" &&
          !entry.first.starts_with("x-")) {
        return true;
      }
    }

    return false;
  }

  // Taken as values, since shadowing renames the members being walked
  static auto member_names(const sourcemeta::core::JSON &schema)
      -> std::vector<sourcemeta::core::JSON::String> {
    std::vector<sourcemeta::core::JSON::String> names;
    names.reserve(schema.size());
    for (const auto &entry : schema.as_object()) {
      names.push_back(entry.first);
    }

    return names;
  }

  // The extension prefix is not a free namespace: core reads two names in it,
  // this dialect's subschema container and the marker that names a dialect. A
  // member prefixed onto either would hand core a schema map or a dialect the
  // author never wrote, so both count as taken and the prefix grows past them
  static auto names_core_reads(const std::string_view name) -> bool {
    return name == DEFINITIONS || name == DIALECT_OVERRIDE_KEYWORD;
  }

  // Core reads the marker as the dialect of the position carrying it, so one
  // naming OpenAPI 3.0 holds that position on this dialect until it is cleared.
  // Nothing else can clear it: every rung of the ladder is gated on a
  // vocabulary this dialect does not have, so the position would keep both the
  // marker and whatever spelling the dialect gave its keywords
  static auto holds_openapi_3_0_override(const sourcemeta::core::JSON &schema)
      -> bool {
    const auto *value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
    return value != nullptr && value->is_string() &&
           names_openapi_3_0_dialect(value->to_string());
  }

  // Whether a shape only the OpenAPI 3.0 dialect admits is still here. Each is
  // either rewritten or shadowed by the transform, so the answer turns false
  // once it has run
  static auto holds_pending_shape(const sourcemeta::core::JSON &schema)
      -> bool {
    const auto *options{schema.try_at("enum")};
    if (options != nullptr && options->is_array() && !options->unique()) {
      return true;
    }

    if ((schema.defines("exclusiveMinimum") && !schema.defines("minimum")) ||
        (schema.defines("exclusiveMaximum") && !schema.defines("maximum"))) {
      return true;
    }

    for (const auto &keyword : {"allOf", "anyOf", "oneOf"}) {
      const auto *branches{
          schema.try_at(sourcemeta::core::JSON::String{keyword})};
      if (branches != nullptr && branches->is_array() && branches->empty()) {
        return true;
      }
    }

    return false;
  }

  // The extension prefix constrains nothing, so a member of this one holds
  // whatever its author put there, while `definitions` reads every member of
  // itself as a schema. Only what is one has somewhere to go
  static auto holds_schema(const sourcemeta::core::JSON &entry) -> bool {
    return entry.is_object() || entry.is_boolean();
  }

  // Neither OpenAPI 3.0 nor Draft 4 has a boolean schema, and yet the walker
  // reads every member of this one as a subschema, so a reference naming a
  // boolean here is asserting something and has to keep asserting it. Draft 4
  // says the same two things with the schema every instance satisfies and the
  // schema none does
  static auto as_object_schema(const sourcemeta::core::JSON &entry)
      -> sourcemeta::core::JSON {
    if (!entry.is_boolean()) {
      return entry;
    }

    auto result{sourcemeta::core::JSON::make_object()};
    if (!entry.to_boolean()) {
      result.assign("not", sourcemeta::core::JSON::make_object());
    }

    return result;
  }

  static auto holds_movable_definitions(const sourcemeta::core::JSON &schema)
      -> bool {
    if (!schema.defines(DEFINITIONS) || !schema.at(DEFINITIONS).is_object()) {
      return false;
    }

    for (const auto &entry : schema.at(DEFINITIONS).as_object()) {
      if (holds_schema(entry.second)) {
        return true;
      }
    }

    return false;
  }

  // Draft 4 keeps under `definitions` what the OpenAPI 3.0 Schema Object keeps
  // here, and reads nothing of a schema under a member it does not know, so
  // every entry that is a schema is taken out for the caller to place and the
  // references reaching one follow it. Anything else stays where it was rather
  // than being carried somewhere that would read it as a schema it is not
  static auto take_definitions(sourcemeta::core::JSON &schema,
                               std::vector<Relocation> &relocations)
      -> sourcemeta::core::JSON {
    auto moved{sourcemeta::core::JSON::make_object()};
    if (!holds_movable_definitions(schema)) {
      return moved;
    }

    auto kept{sourcemeta::core::JSON::make_object()};
    for (const auto &entry : schema.at(DEFINITIONS).as_object()) {
      if (!holds_schema(entry.second)) {
        kept.assign(entry.first, entry.second);
        continue;
      }

      relocations.emplace_back(
          sourcemeta::core::Pointer{DEFINITIONS, entry.first},
          sourcemeta::core::Pointer{"definitions", entry.first});
      moved.assign(entry.first, as_object_schema(entry.second));
    }

    schema.erase(DEFINITIONS);
    if (!kept.empty()) {
      schema.assign(DEFINITIONS, std::move(kept));
    }

    return moved;
  }

  const bool keeps_read_only_;
  const bool keeps_deprecated_;
  const bool keeps_openapi_annotations_;
  const bool keeps_default_mapping_;
  mutable std::unordered_map<std::string, std::string> renames_;
  mutable std::vector<Relocation> relocations_;
};
