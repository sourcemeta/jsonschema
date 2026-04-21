class ValidExamples final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ValidExamples(Compiler compiler = default_schema_compiler)
      : SchemaTransformRule{"valid_examples",
                            "Only include instances in the `examples` array "
                            "that validate against the schema"},
        compiler_{std::move(compiler)} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const
      -> SchemaTransformRule::Result override {
    using Known = Vocabularies::Known;
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Known::JSON_Schema_2020_12_Meta_Data,
                                   Known::JSON_Schema_2019_09_Meta_Data,
                                   Known::JSON_Schema_Draft_7,
                                   Known::JSON_Schema_Draft_6}) &&
        schema.is_object() && schema.defines("examples") &&
        schema.at("examples").is_array() && !schema.at("examples").empty());

    if (vocabularies.contains_any({Known::JSON_Schema_Draft_7,
                                   Known::JSON_Schema_Draft_6,
                                   Known::JSON_Schema_Draft_4})) {
      ONLY_CONTINUE_IF(!schema.defines("$ref"));
    }

    // TODO(C++23): Use std::views::enumerate when available in libc++
    std::size_t cursor{0};

    if (frame.standalone()) {
      const auto base{frame.uri(location.pointer)};
      assert(base.has_value());
      Template schema_template;
      try {
        schema_template = compile(root, walker, resolver, this->compiler_,
                                  frame, base.value().get(), Mode::Exhaustive);
      } catch (const CompilerReferenceTargetNotSchemaError &) {
        throw;
      } catch (const sourcemeta::core::SchemaVocabularyError &) {
        throw;
      } catch (...) {
        return false;
      }

      for (const auto &example : schema.at("examples").as_array()) {
        SimpleOutput output{example};
        Evaluator evaluator;
        const auto result{
            evaluator.validate(schema_template, example, std::ref(output))};
        if (!result) {
          std::ostringstream message;
          message << "Invalid example instance at index " << cursor << "\n";
          for (const auto &entry : output) {
            message << "  " << entry.message << "\n";
            message << "  "
                    << "  at instance location \"";
            sourcemeta::core::stringify(entry.instance_location, message);
            message << "\"\n";
            message << "  "
                    << "  at evaluate path \"";
            sourcemeta::core::stringify(entry.evaluate_path, message);
            message << "\"\n";
          }

          return {{{"examples", cursor}}, std::move(message).str()};
        }

        cursor += 1;
      }

      return false;
    }

    const auto &root_base_dialect{
        frame.traverse(frame.root()).value_or(location).get().base_dialect};
    std::string_view default_id{location.base};
    if (!sourcemeta::core::identify(root, root_base_dialect).empty() ||
        default_id.empty()) {
      default_id = "";
    }

    sourcemeta::core::WeakPointer base;
    const auto subschema{
        sourcemeta::core::wrap(root, frame, location, resolver, base)};
    Template schema_template;
    try {
      schema_template = compile(subschema, walker, resolver, this->compiler_,
                                Mode::Exhaustive, location.dialect, default_id);
    } catch (const CompilerReferenceTargetNotSchemaError &) {
      throw;
    } catch (const sourcemeta::core::SchemaVocabularyError &) {
      throw;
    } catch (...) {
      return false;
    }

    for (const auto &example : schema.at("examples").as_array()) {
      SimpleOutput output{example, base};
      Evaluator evaluator;
      const auto result{
          evaluator.validate(schema_template, example, std::ref(output))};
      if (!result) {
        std::ostringstream message;
        message << "Invalid example instance at index " << cursor << "\n";
        for (const auto &entry : output) {
          message << "  " << entry.message << "\n";
          message << "  "
                  << "  at instance location \"";
          sourcemeta::core::stringify(entry.instance_location, message);
          message << "\"\n";
          message << "  "
                  << "  at evaluate path \"";
          sourcemeta::core::stringify(entry.evaluate_path, message);
          message << "\"\n";
        }

        return {{{"examples", cursor}}, std::move(message).str()};
      }

      cursor += 1;
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("examples");
  }

private:
  const Compiler compiler_;
};
