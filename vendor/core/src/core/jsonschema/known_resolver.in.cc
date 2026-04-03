#include <sourcemeta/core/jsonschema.h>

#include <cstdint> // std::uint8_t

enum class KnownSchema : std::uint8_t {
  JSONSCHEMA_2020_12,
  HYPERSCHEMA_2020_12,
  JSONSCHEMA_2020_12_APPLICATOR,
  JSONSCHEMA_2020_12_CONTENT,
  JSONSCHEMA_2020_12_CORE,
  JSONSCHEMA_2020_12_FORMAT_ANNOTATION,
  JSONSCHEMA_2020_12_FORMAT_ASSERTION,
  JSONSCHEMA_2020_12_HYPER_SCHEMA,
  JSONSCHEMA_2020_12_META_DATA,
  JSONSCHEMA_2020_12_UNEVALUATED,
  JSONSCHEMA_2020_12_VALIDATION,
  LINKS_2020_12,
  JSONSCHEMA_2020_12_OUTPUT,

  JSONSCHEMA_2019_09,
  HYPERSCHEMA_2019_09,
  JSONSCHEMA_2019_09_APPLICATOR,
  JSONSCHEMA_2019_09_CONTENT,
  JSONSCHEMA_2019_09_CORE,
  JSONSCHEMA_2019_09_FORMAT,
  JSONSCHEMA_2019_09_HYPER_SCHEMA,
  JSONSCHEMA_2019_09_META_DATA,
  JSONSCHEMA_2019_09_VALIDATION,
  LINKS_2019_09,
  JSONSCHEMA_2019_09_OUTPUT,
  HYPERSCHEMA_2019_09_OUTPUT,

  JSONSCHEMA_DRAFT7,
  HYPERSCHEMA_DRAFT7,
  LINKS_DRAFT7,
  HYPERSCHEMA_DRAFT7_OUTPUT,

  JSONSCHEMA_DRAFT6,
  HYPERSCHEMA_DRAFT6,
  LINKS_DRAFT6,

  JSONSCHEMA_DRAFT4,
  HYPERSCHEMA_DRAFT4,
  LINKS_DRAFT4,

  JSONSCHEMA_DRAFT3,
  HYPERSCHEMA_DRAFT3,
  LINKS_DRAFT3,
  JSON_REF_DRAFT3,

  JSONSCHEMA_DRAFT2,
  HYPERSCHEMA_DRAFT2,
  LINKS_DRAFT2,
  JSON_REF_DRAFT2,

  JSONSCHEMA_DRAFT1,
  HYPERSCHEMA_DRAFT1,
  LINKS_DRAFT1,
  JSON_REF_DRAFT1,

  JSONSCHEMA_DRAFT0,
  HYPERSCHEMA_DRAFT0,
  LINKS_DRAFT0,
  JSON_REF_DRAFT0,

  OAS_3_2_DIALECT_2025_09_17,
  OAS_3_2_META_2025_09_17,

  OAS_3_1_DIALECT_BASE,
  OAS_3_1_META_BASE,

  UNKNOWN
};

static auto parse_identifier(const std::string_view identifier) -> KnownSchema {
  // JSON Schema 2020-12
  if (identifier == "https://json-schema.org/draft/2020-12/schema" ||
      identifier == "https://json-schema.org/draft/2020-12/schema#" ||
      identifier == "http://json-schema.org/draft/2020-12/schema" ||
      identifier == "http://json-schema.org/draft/2020-12/schema#") {
    return KnownSchema::JSONSCHEMA_2020_12;
  } else if (identifier ==
                 "https://json-schema.org/draft/2020-12/hyper-schema" ||
             identifier ==
                 "https://json-schema.org/draft/2020-12/hyper-schema#" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/hyper-schema" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/hyper-schema#") {
    return KnownSchema::HYPERSCHEMA_2020_12;
  } else if (identifier ==
                 "https://json-schema.org/draft/2020-12/meta/applicator" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/meta/applicator") {
    return KnownSchema::JSONSCHEMA_2020_12_APPLICATOR;
  } else if (identifier ==
                 "https://json-schema.org/draft/2020-12/meta/content" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/meta/content") {
    return KnownSchema::JSONSCHEMA_2020_12_CONTENT;
  } else if (identifier == "https://json-schema.org/draft/2020-12/meta/core" ||
             identifier == "http://json-schema.org/draft/2020-12/meta/core") {
    return KnownSchema::JSONSCHEMA_2020_12_CORE;
  } else if (
      identifier ==
          "https://json-schema.org/draft/2020-12/meta/format-annotation" ||
      identifier ==
          "http://json-schema.org/draft/2020-12/meta/format-annotation") {
    return KnownSchema::JSONSCHEMA_2020_12_FORMAT_ANNOTATION;
  } else if (identifier == "https://json-schema.org/draft/2020-12/meta/"
                           "format-assertion" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/meta/format-assertion") {
    return KnownSchema::JSONSCHEMA_2020_12_FORMAT_ASSERTION;
  } else if (identifier ==
                 "https://json-schema.org/draft/2020-12/meta/hyper-schema" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/meta/hyper-schema") {
    return KnownSchema::JSONSCHEMA_2020_12_HYPER_SCHEMA;
  } else if (identifier ==
                 "https://json-schema.org/draft/2020-12/meta/meta-data" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/meta/meta-data") {
    return KnownSchema::JSONSCHEMA_2020_12_META_DATA;
  } else if (identifier ==
                 "https://json-schema.org/draft/2020-12/meta/unevaluated" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/meta/unevaluated") {
    return KnownSchema::JSONSCHEMA_2020_12_UNEVALUATED;
  } else if (identifier ==
                 "https://json-schema.org/draft/2020-12/meta/validation" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/meta/validation") {
    return KnownSchema::JSONSCHEMA_2020_12_VALIDATION;
  } else if (identifier == "https://json-schema.org/draft/2020-12/links" ||
             identifier == "http://json-schema.org/draft/2020-12/links") {
    return KnownSchema::LINKS_2020_12;
  } else if (identifier ==
                 "https://json-schema.org/draft/2020-12/output/schema" ||
             identifier ==
                 "http://json-schema.org/draft/2020-12/output/schema") {
    return KnownSchema::JSONSCHEMA_2020_12_OUTPUT;

    // JSON Schema 2019-09
  } else if (identifier == "https://json-schema.org/draft/2019-09/schema" ||
             identifier == "https://json-schema.org/draft/2019-09/schema#" ||
             identifier == "http://json-schema.org/draft/2019-09/schema" ||
             identifier == "http://json-schema.org/draft/2019-09/schema#") {
    return KnownSchema::JSONSCHEMA_2019_09;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/hyper-schema" ||
             identifier ==
                 "https://json-schema.org/draft/2019-09/hyper-schema#" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/hyper-schema" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/hyper-schema#") {
    return KnownSchema::HYPERSCHEMA_2019_09;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/meta/applicator" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/meta/applicator") {
    return KnownSchema::JSONSCHEMA_2019_09_APPLICATOR;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/meta/content" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/meta/content") {
    return KnownSchema::JSONSCHEMA_2019_09_CONTENT;
  } else if (identifier == "https://json-schema.org/draft/2019-09/meta/core" ||
             identifier == "http://json-schema.org/draft/2019-09/meta/core") {
    return KnownSchema::JSONSCHEMA_2019_09_CORE;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/meta/format" ||
             identifier == "http://json-schema.org/draft/2019-09/meta/format") {
    return KnownSchema::JSONSCHEMA_2019_09_FORMAT;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/meta/hyper-schema" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/meta/hyper-schema") {
    return KnownSchema::JSONSCHEMA_2019_09_HYPER_SCHEMA;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/meta/meta-data" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/meta/meta-data") {
    return KnownSchema::JSONSCHEMA_2019_09_META_DATA;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/meta/validation" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/meta/validation") {
    return KnownSchema::JSONSCHEMA_2019_09_VALIDATION;
  } else if (identifier == "https://json-schema.org/draft/2019-09/links" ||
             identifier == "http://json-schema.org/draft/2019-09/links") {
    return KnownSchema::LINKS_2019_09;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/output/schema" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/output/schema") {
    return KnownSchema::JSONSCHEMA_2019_09_OUTPUT;
  } else if (identifier ==
                 "https://json-schema.org/draft/2019-09/output/hyper-schema" ||
             identifier ==
                 "http://json-schema.org/draft/2019-09/output/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_2019_09_OUTPUT;

    // JSON Schema Draft7
  } else if (identifier == "http://json-schema.org/draft-07/schema#" ||
             identifier == "http://json-schema.org/draft-07/schema" ||
             identifier == "https://json-schema.org/draft-07/schema#" ||
             identifier == "https://json-schema.org/draft-07/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT7;
  } else if (identifier == "http://json-schema.org/draft-07/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-07/hyper-schema" ||
             identifier == "https://json-schema.org/draft-07/hyper-schema#" ||
             identifier == "https://json-schema.org/draft-07/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT7;
  } else if (identifier == "http://json-schema.org/draft-07/links#" ||
             identifier == "http://json-schema.org/draft-07/links" ||
             identifier == "https://json-schema.org/draft-07/links#" ||
             identifier == "https://json-schema.org/draft-07/links") {
    return KnownSchema::LINKS_DRAFT7;
  } else if (identifier ==
                 "http://json-schema.org/draft-07/hyper-schema-output" ||
             identifier ==
                 "https://json-schema.org/draft-07/hyper-schema-output") {
    return KnownSchema::HYPERSCHEMA_DRAFT7_OUTPUT;

    // JSON Schema Draft6
  } else if (identifier == "http://json-schema.org/draft-06/schema#" ||
             identifier == "http://json-schema.org/draft-06/schema" ||
             identifier == "https://json-schema.org/draft-06/schema#" ||
             identifier == "https://json-schema.org/draft-06/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT6;
  } else if (identifier == "http://json-schema.org/draft-06/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-06/hyper-schema" ||
             identifier == "https://json-schema.org/draft-06/hyper-schema#" ||
             identifier == "https://json-schema.org/draft-06/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT6;
  } else if (identifier == "http://json-schema.org/draft-06/links#" ||
             identifier == "http://json-schema.org/draft-06/links" ||
             identifier == "https://json-schema.org/draft-06/links#" ||
             identifier == "https://json-schema.org/draft-06/links") {
    return KnownSchema::LINKS_DRAFT6;

    // JSON Schema Draft4
  } else if (identifier == "http://json-schema.org/draft-04/schema#" ||
             identifier == "http://json-schema.org/draft-04/schema" ||
             identifier == "https://json-schema.org/draft-04/schema#" ||
             identifier == "https://json-schema.org/draft-04/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT4;
  } else if (identifier == "http://json-schema.org/draft-04/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-04/hyper-schema" ||
             identifier == "https://json-schema.org/draft-04/hyper-schema#" ||
             identifier == "https://json-schema.org/draft-04/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT4;
  } else if (identifier == "http://json-schema.org/draft-04/links#" ||
             identifier == "http://json-schema.org/draft-04/links" ||
             identifier == "https://json-schema.org/draft-04/links#" ||
             identifier == "https://json-schema.org/draft-04/links") {
    return KnownSchema::LINKS_DRAFT4;

    // JSON Schema Draft3
  } else if (identifier == "http://json-schema.org/draft-03/schema#" ||
             identifier == "http://json-schema.org/draft-03/schema" ||
             identifier == "https://json-schema.org/draft-03/schema#" ||
             identifier == "https://json-schema.org/draft-03/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT3;
  } else if (identifier == "http://json-schema.org/draft-03/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-03/hyper-schema" ||
             identifier == "https://json-schema.org/draft-03/hyper-schema#" ||
             identifier == "https://json-schema.org/draft-03/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT3;
  } else if (identifier == "http://json-schema.org/draft-03/links#" ||
             identifier == "http://json-schema.org/draft-03/links" ||
             identifier == "https://json-schema.org/draft-03/links#" ||
             identifier == "https://json-schema.org/draft-03/links") {
    return KnownSchema::LINKS_DRAFT3;
  } else if (identifier == "http://json-schema.org/draft-03/json-ref#" ||
             identifier == "http://json-schema.org/draft-03/json-ref" ||
             identifier == "https://json-schema.org/draft-03/json-ref#" ||
             identifier == "https://json-schema.org/draft-03/json-ref") {
    return KnownSchema::JSON_REF_DRAFT3;

    // JSON Schema Draft2
  } else if (identifier == "http://json-schema.org/draft-02/schema#" ||
             identifier == "http://json-schema.org/draft-02/schema" ||
             identifier == "https://json-schema.org/draft-02/schema#" ||
             identifier == "https://json-schema.org/draft-02/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT2;
  } else if (identifier == "http://json-schema.org/draft-02/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-02/hyper-schema" ||
             identifier == "https://json-schema.org/draft-02/hyper-schema#" ||
             identifier == "https://json-schema.org/draft-02/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT2;
  } else if (identifier == "http://json-schema.org/draft-02/links#" ||
             identifier == "http://json-schema.org/draft-02/links" ||
             identifier == "https://json-schema.org/draft-02/links#" ||
             identifier == "https://json-schema.org/draft-02/links") {
    return KnownSchema::LINKS_DRAFT2;
  } else if (identifier == "http://json-schema.org/draft-02/json-ref#" ||
             identifier == "http://json-schema.org/draft-02/json-ref" ||
             identifier == "https://json-schema.org/draft-02/json-ref#" ||
             identifier == "https://json-schema.org/draft-02/json-ref") {
    return KnownSchema::JSON_REF_DRAFT2;

    // JSON Schema Draft1
  } else if (identifier == "http://json-schema.org/draft-01/schema#" ||
             identifier == "http://json-schema.org/draft-01/schema" ||
             identifier == "https://json-schema.org/draft-01/schema#" ||
             identifier == "https://json-schema.org/draft-01/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT1;
  } else if (identifier == "http://json-schema.org/draft-01/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-01/hyper-schema" ||
             identifier == "https://json-schema.org/draft-01/hyper-schema#" ||
             identifier == "https://json-schema.org/draft-01/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT1;
  } else if (identifier == "http://json-schema.org/draft-01/links#" ||
             identifier == "http://json-schema.org/draft-01/links" ||
             identifier == "https://json-schema.org/draft-01/links#" ||
             identifier == "https://json-schema.org/draft-01/links") {
    return KnownSchema::LINKS_DRAFT1;
  } else if (identifier == "http://json-schema.org/draft-01/json-ref#" ||
             identifier == "http://json-schema.org/draft-01/json-ref" ||
             identifier == "https://json-schema.org/draft-01/json-ref#" ||
             identifier == "https://json-schema.org/draft-01/json-ref") {
    return KnownSchema::JSON_REF_DRAFT1;

    // JSON Schema Draft0
  } else if (identifier == "http://json-schema.org/draft-00/schema#" ||
             identifier == "http://json-schema.org/draft-00/schema" ||
             identifier == "https://json-schema.org/draft-00/schema#" ||
             identifier == "https://json-schema.org/draft-00/schema") {
    return KnownSchema::JSONSCHEMA_DRAFT0;
  } else if (identifier == "http://json-schema.org/draft-00/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-00/hyper-schema" ||
             identifier == "https://json-schema.org/draft-00/hyper-schema#" ||
             identifier == "https://json-schema.org/draft-00/hyper-schema") {
    return KnownSchema::HYPERSCHEMA_DRAFT0;
  } else if (identifier == "http://json-schema.org/draft-00/links#" ||
             identifier == "http://json-schema.org/draft-00/links" ||
             identifier == "https://json-schema.org/draft-00/links#" ||
             identifier == "https://json-schema.org/draft-00/links") {
    return KnownSchema::LINKS_DRAFT0;
  } else if (identifier == "http://json-schema.org/draft-00/json-ref#" ||
             identifier == "http://json-schema.org/draft-00/json-ref" ||
             identifier == "https://json-schema.org/draft-00/json-ref#" ||
             identifier == "https://json-schema.org/draft-00/json-ref") {
    return KnownSchema::JSON_REF_DRAFT0;

    // OpenAPI v3.2
  } else if (identifier ==
             "https://spec.openapis.org/oas/3.2/dialect/2025-09-17") {
    return KnownSchema::OAS_3_2_DIALECT_2025_09_17;
  } else if (identifier ==
             "https://spec.openapis.org/oas/3.2/meta/2025-09-17") {
    return KnownSchema::OAS_3_2_META_2025_09_17;

    // OpenAPI v3.1
  } else if (identifier == "https://spec.openapis.org/oas/3.1/dialect/base") {
    return KnownSchema::OAS_3_1_DIALECT_BASE;
  } else if (identifier == "https://spec.openapis.org/oas/3.1/meta/base") {
    return KnownSchema::OAS_3_1_META_BASE;
  }

  return KnownSchema::UNKNOWN;
}

auto sourcemeta::core::schema_resolver(const std::string_view identifier)
    -> std::optional<sourcemeta::core::JSON> {
  switch (parse_identifier(identifier)) {
    case KnownSchema::JSONSCHEMA_2020_12:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12@)EOF");
    case KnownSchema::HYPERSCHEMA_2020_12:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_2020_12@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_APPLICATOR:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_APPLICATOR@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_CONTENT:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_CONTENT@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_CORE:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_CORE@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_FORMAT_ANNOTATION:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_FORMAT_ANNOTATION@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_FORMAT_ASSERTION:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_FORMAT_ASSERTION@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_HYPER_SCHEMA:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_HYPER_SCHEMA@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_META_DATA:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_META_DATA@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_UNEVALUATED:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_UNEVALUATED@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_VALIDATION:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_VALIDATION@)EOF");
    case KnownSchema::LINKS_2020_12:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_LINKS_2020_12@)EOF");
    case KnownSchema::JSONSCHEMA_2020_12_OUTPUT:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_OUTPUT@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09@)EOF");
    case KnownSchema::HYPERSCHEMA_2019_09:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_2019_09@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09_APPLICATOR:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_APPLICATOR@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09_CONTENT:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_CONTENT@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09_CORE:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_CORE@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09_FORMAT:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_FORMAT@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09_HYPER_SCHEMA:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_HYPER_SCHEMA@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09_META_DATA:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_META_DATA@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09_VALIDATION:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_VALIDATION@)EOF");
    case KnownSchema::LINKS_2019_09:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_LINKS_2019_09@)EOF");
    case KnownSchema::JSONSCHEMA_2019_09_OUTPUT:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_OUTPUT@)EOF");
    case KnownSchema::HYPERSCHEMA_2019_09_OUTPUT:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_2019_09_OUTPUT@)EOF");
    case KnownSchema::JSONSCHEMA_DRAFT7:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT7@)EOF");
    case KnownSchema::HYPERSCHEMA_DRAFT7:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT7@)EOF");
    case KnownSchema::LINKS_DRAFT7:
      return sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT7@)EOF");
    case KnownSchema::HYPERSCHEMA_DRAFT7_OUTPUT:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT7_OUTPUT@)EOF");
    case KnownSchema::JSONSCHEMA_DRAFT6:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT6@)EOF");
    case KnownSchema::HYPERSCHEMA_DRAFT6:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT6@)EOF");
    case KnownSchema::LINKS_DRAFT6:
      return sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT6@)EOF");
    case KnownSchema::JSONSCHEMA_DRAFT4:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT4@)EOF");
    case KnownSchema::HYPERSCHEMA_DRAFT4:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT4@)EOF");
    case KnownSchema::LINKS_DRAFT4:
      return sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT4@)EOF");
    case KnownSchema::JSONSCHEMA_DRAFT3:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT3@)EOF");
    case KnownSchema::HYPERSCHEMA_DRAFT3:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT3@)EOF");
    case KnownSchema::LINKS_DRAFT3:
      return sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT3@)EOF");
    case KnownSchema::JSON_REF_DRAFT3:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSON_REF_DRAFT3@)EOF");
    case KnownSchema::JSONSCHEMA_DRAFT2:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT2@)EOF");
    case KnownSchema::HYPERSCHEMA_DRAFT2:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT2@)EOF");
    case KnownSchema::LINKS_DRAFT2:
      return sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT2@)EOF");
    case KnownSchema::JSON_REF_DRAFT2:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSON_REF_DRAFT2@)EOF");
    case KnownSchema::JSONSCHEMA_DRAFT1:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT1@)EOF");
    case KnownSchema::HYPERSCHEMA_DRAFT1:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT1@)EOF");
    case KnownSchema::LINKS_DRAFT1:
      return sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT1@)EOF");
    case KnownSchema::JSON_REF_DRAFT1:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSON_REF_DRAFT1@)EOF");
    case KnownSchema::JSONSCHEMA_DRAFT0:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT0@)EOF");
    case KnownSchema::HYPERSCHEMA_DRAFT0:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT0@)EOF");
    case KnownSchema::LINKS_DRAFT0:
      return sourcemeta::core::parse_json(R"EOF(@METASCHEMA_LINKS_DRAFT0@)EOF");
    case KnownSchema::JSON_REF_DRAFT0:
      return sourcemeta::core::parse_json(
          R"EOF(@METASCHEMA_JSON_REF_DRAFT0@)EOF");
    case KnownSchema::OAS_3_2_DIALECT_2025_09_17:
      return sourcemeta::core::parse_json(
          R"EOF(@OPENAPI_OAS_3_2_DIALECT_2025_09_17@)EOF");
    case KnownSchema::OAS_3_2_META_2025_09_17:
      return sourcemeta::core::parse_json(
          R"EOF(@OPENAPI_OAS_3_2_META_2025_09_17@)EOF");
    case KnownSchema::OAS_3_1_DIALECT_BASE:
      return sourcemeta::core::parse_json(
          R"EOF(@OPENAPI_OAS_3_1_DIALECT_BASE@)EOF");
    case KnownSchema::OAS_3_1_META_BASE:
      return sourcemeta::core::parse_json(
          R"EOF(@OPENAPI_OAS_3_1_META_BASE@)EOF");
    case KnownSchema::UNKNOWN:
      return std::nullopt;
  }

  return std::nullopt;
}

auto sourcemeta::core::is_known_schema(
    const std::string_view identifier) noexcept -> bool {
  return parse_identifier(identifier) != KnownSchema::UNKNOWN;
}
