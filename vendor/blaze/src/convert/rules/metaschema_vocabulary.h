class MetaschemaVocabulary final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MetaschemaVocabulary() : SchemaTransformRule{"metaschema_vocabulary"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &,
            const bool is_metaschema) const -> bool override {
    ONLY_CONTINUE_IF(schema.is_object() && !schema.defines("$vocabulary"));

    // Whichever dialect the meta-schema ends up on is the one whose
    // vocabularies it has to declare, rather than the one the caller asked to
    // convert to
    this->emits_2020_12_ = vocabularies.contains(
        SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE);
    ONLY_CONTINUE_IF(this->emits_2020_12_ ||
                     vocabularies.contains(
                         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE));

    return (is_metaschema && location.pointer.empty()) ||
           is_metaschema_target(schema, frame, location.pointer);
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (this->emits_2020_12_) {
      synthesize_vocabulary(schema, VOCABULARIES_2020_12);
    } else {
      synthesize_vocabulary(schema, VOCABULARIES_2019_09);
    }
  }

private:
  using Vocabulary = std::pair<std::string_view, bool>;

  static constexpr std::array<Vocabulary, 6> VOCABULARIES_2019_09{
      {{"https://json-schema.org/draft/2019-09/vocab/core", true},
       {"https://json-schema.org/draft/2019-09/vocab/applicator", true},
       {"https://json-schema.org/draft/2019-09/vocab/validation", true},
       {"https://json-schema.org/draft/2019-09/vocab/meta-data", true},
       {"https://json-schema.org/draft/2019-09/vocab/format", false},
       {"https://json-schema.org/draft/2019-09/vocab/content", true}}};

  static constexpr std::array<Vocabulary, 7> VOCABULARIES_2020_12{
      {{"https://json-schema.org/draft/2020-12/vocab/core", true},
       {"https://json-schema.org/draft/2020-12/vocab/applicator", true},
       {"https://json-schema.org/draft/2020-12/vocab/unevaluated", true},
       {"https://json-schema.org/draft/2020-12/vocab/validation", true},
       {"https://json-schema.org/draft/2020-12/vocab/meta-data", true},
       {"https://json-schema.org/draft/2020-12/vocab/format-annotation", false},
       {"https://json-schema.org/draft/2020-12/vocab/content", true}}};

  mutable bool emits_2020_12_{false};

  template <std::size_t Size>
  static auto synthesize_vocabulary(sourcemeta::core::JSON &schema,
                                    const std::array<Vocabulary, Size> &entries)
      -> void {
    std::string_view anchor;
    if (schema.defines("$id")) {
      anchor = "$id";
    } else if (schema.defines("$schema")) {
      anchor = "$schema";
    }

    const std::string *next_key{nullptr};
    if (!anchor.empty()) {
      bool found_anchor{false};
      for (const auto &entry : schema.as_object()) {
        if (found_anchor) {
          next_key = &entry.first;
          break;
        }
        if (entry.first == anchor) {
          found_anchor = true;
        }
      }
    }

    if (next_key != nullptr) {
      schema.try_assign_before(
          "$vocabulary", sourcemeta::core::JSON::make_object(), *next_key);
    } else {
      schema.assign_assume_new("$vocabulary",
                               sourcemeta::core::JSON::make_object());
    }

    auto &vocabularies{schema.at("$vocabulary")};
    for (const auto &[uri, required] : entries) {
      vocabularies.assign_assume_new(std::string{uri},
                                     sourcemeta::core::JSON{required});
    }
  }
};
