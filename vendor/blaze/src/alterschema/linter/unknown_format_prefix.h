class UnknownFormatPrefix final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnknownFormatPrefix()
      : SchemaTransformRule{
            "unknown_format_prefix",
            "For interoperability purposes, the JSON Schema specification "
            "advises against the use of "
            "`format` values that are not explicitly defined by the "
            "specification"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("format"));
    const auto &format_value{schema.at("format")};
    ONLY_CONTINUE_IF(format_value.is_string());

    const auto *recognized{recognized_formats_for(vocabularies)};
    if (recognized == nullptr) {
      return false;
    }

    if (recognized->contains(format_value.to_string())) {
      return false;
    }

    return applies_to_keywords("format");
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    std::string prefixed_name{"x-format"};
    while (schema.defines(prefixed_name)) {
      prefixed_name.insert(0, "x-");
    }
    schema.rename("format", std::move(prefixed_name));
  }

private:
  static auto recognized_formats_for(
      const sourcemeta::blaze::SchemaVocabularies &vocabularies)
      -> const std::unordered_set<std::string_view> * {
    using Known = sourcemeta::blaze::SchemaVocabularies::Known;
    if (vocabularies.contains_any(
            {Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER})) {
      return &DRAFT_3_FORMATS;
    }
    if (vocabularies.contains_any(
            {Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER})) {
      return &DRAFT_4_FORMATS;
    }
    if (vocabularies.contains_any(
            {Known::JSON_SCHEMA_DRAFT_6, Known::JSON_SCHEMA_DRAFT_6_HYPER})) {
      return &DRAFT_6_FORMATS;
    }
    if (vocabularies.contains_any(
            {Known::JSON_SCHEMA_DRAFT_7, Known::JSON_SCHEMA_DRAFT_7_HYPER})) {
      return &DRAFT_7_FORMATS;
    }
    if (vocabularies.contains(Known::JSON_SCHEMA_2019_09_FORMAT)) {
      return &DRAFT_2019_09_FORMATS;
    }
    if (vocabularies.contains(Known::JSON_SCHEMA_2020_12_FORMAT_ANNOTATION) ||
        vocabularies.contains(Known::JSON_SCHEMA_2020_12_FORMAT_ASSERTION)) {
      return &DRAFT_2020_12_FORMATS;
    }
    return nullptr;
  }

  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::unordered_set<std::string_view> DRAFT_3_FORMATS{
      "date-time",  "date",  "time",     "utc-millisec", "regex",
      "color",      "style", "phone",    "uri",          "email",
      "ip-address", "ipv6",  "host-name"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::unordered_set<std::string_view> DRAFT_4_FORMATS{
      "date-time", "email", "hostname", "ipv4", "ipv6", "uri"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::unordered_set<std::string_view> DRAFT_6_FORMATS{
      "date-time", "email",         "hostname",     "ipv4",        "ipv6",
      "uri",       "uri-reference", "uri-template", "json-pointer"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::unordered_set<std::string_view> DRAFT_7_FORMATS{
      "date-time",     "date",         "time",          "email",
      "idn-email",     "hostname",     "idn-hostname",  "ipv4",
      "ipv6",          "uri",          "uri-reference", "iri",
      "iri-reference", "uri-template", "json-pointer",  "relative-json-pointer",
      "regex"};
  static inline const std::unordered_set<std::string_view>
      // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
      DRAFT_2019_09_FORMATS{
          "date-time",    "date",          "time",
          "duration",     "email",         "idn-email",
          "hostname",     "idn-hostname",  "ipv4",
          "ipv6",         "uri",           "uri-reference",
          "iri",          "iri-reference", "uuid",
          "uri-template", "json-pointer",  "relative-json-pointer",
          "regex"};
  static inline const std::unordered_set<std::string_view>
      // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
      DRAFT_2020_12_FORMATS{
          "date-time",    "date",          "time",
          "duration",     "email",         "idn-email",
          "hostname",     "idn-hostname",  "ipv4",
          "ipv6",         "uri",           "uri-reference",
          "iri",          "iri-reference", "uuid",
          "uri-template", "json-pointer",  "relative-json-pointer",
          "regex"};
};
