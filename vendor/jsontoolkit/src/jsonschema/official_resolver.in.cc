#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>

auto sourcemeta::jsontoolkit::official_resolver(std::string_view identifier)
    -> std::future<std::optional<sourcemeta::jsontoolkit::JSON>> {
  std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;

  // JSON Schema 2020-12
  if (identifier == "https://json-schema.org/draft/2020-12/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_2020_12@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/meta/applicator") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_APPLICATOR@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/meta/content") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_CONTENT@)EOF"));
  } else if (identifier == "https://json-schema.org/draft/2020-12/meta/core") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_CORE@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/meta/format-annotation") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_FORMAT_ANNOTATION@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/meta/format-assertion") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_FORMAT_ASSERTION@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/meta/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_HYPER_SCHEMA@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/meta/meta-data") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_META_DATA@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/meta/unevaluated") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_UNEVALUATED@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/meta/validation") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_VALIDATION@)EOF"));
  } else if (identifier == "https://json-schema.org/draft/2020-12/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_2020_12@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2020-12/output/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2020_12_OUTPUT@)EOF"));

    // JSON Schema 2019-09
  } else if (identifier == "https://json-schema.org/draft/2019-09/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_2019_09@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/meta/applicator") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_APPLICATOR@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/meta/content") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_CONTENT@)EOF"));
  } else if (identifier == "https://json-schema.org/draft/2019-09/meta/core") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_CORE@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/meta/format") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_FORMAT@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/meta/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_HYPER_SCHEMA@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/meta/meta-data") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_META_DATA@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/meta/validation") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_VALIDATION@)EOF"));
  } else if (identifier == "https://json-schema.org/draft/2019-09/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_2019_09@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/output/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_2019_09_OUTPUT@)EOF"));
  } else if (identifier ==
             "https://json-schema.org/draft/2019-09/output/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_2019_09_OUTPUT@)EOF"));

    // JSON Schema Draft7
  } else if (identifier == "http://json-schema.org/draft-07/schema#" ||
             identifier == "http://json-schema.org/draft-07/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT7@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-07/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-07/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT7@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-07/links#" ||
             identifier == "http://json-schema.org/draft-07/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_DRAFT7@)EOF"));
  } else if (identifier ==
             "http://json-schema.org/draft-07/hyper-schema-output") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT7_OUTPUT@)EOF"));

    // JSON Schema Draft6
  } else if (identifier == "http://json-schema.org/draft-06/schema#" ||
             identifier == "http://json-schema.org/draft-06/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT6@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-06/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-06/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT6@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-06/links#" ||
             identifier == "http://json-schema.org/draft-06/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_DRAFT6@)EOF"));

    // JSON Schema Draft4
  } else if (identifier == "http://json-schema.org/draft-04/schema#" ||
             identifier == "http://json-schema.org/draft-04/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT4@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-04/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-04/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT4@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-04/links#" ||
             identifier == "http://json-schema.org/draft-04/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_DRAFT4@)EOF"));

    // JSON Schema Draft3
  } else if (identifier == "http://json-schema.org/draft-03/schema#" ||
             identifier == "http://json-schema.org/draft-03/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT3@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-03/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-03/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT3@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-03/links#" ||
             identifier == "http://json-schema.org/draft-03/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_DRAFT3@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-03/json-ref#" ||
             identifier == "http://json-schema.org/draft-03/json-ref") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSON_REF_DRAFT3@)EOF"));

    // JSON Schema Draft2
  } else if (identifier == "http://json-schema.org/draft-02/schema#" ||
             identifier == "http://json-schema.org/draft-02/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT2@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-02/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-02/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT2@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-02/links#" ||
             identifier == "http://json-schema.org/draft-02/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_DRAFT2@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-02/json-ref#" ||
             identifier == "http://json-schema.org/draft-02/json-ref") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSON_REF_DRAFT2@)EOF"));

    // JSON Schema Draft1
  } else if (identifier == "http://json-schema.org/draft-01/schema#" ||
             identifier == "http://json-schema.org/draft-01/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT1@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-01/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-01/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT1@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-01/links#" ||
             identifier == "http://json-schema.org/draft-01/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_DRAFT1@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-01/json-ref#" ||
             identifier == "http://json-schema.org/draft-01/json-ref") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSON_REF_DRAFT1@)EOF"));

    // JSON Schema Draft0
  } else if (identifier == "http://json-schema.org/draft-00/schema#" ||
             identifier == "http://json-schema.org/draft-00/schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSONSCHEMA_DRAFT0@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-00/hyper-schema#" ||
             identifier == "http://json-schema.org/draft-00/hyper-schema") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_HYPERSCHEMA_DRAFT0@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-00/links#" ||
             identifier == "http://json-schema.org/draft-00/links") {
    promise.set_value(
        sourcemeta::jsontoolkit::parse(R"EOF(@METASCHEMA_LINKS_DRAFT0@)EOF"));
  } else if (identifier == "http://json-schema.org/draft-00/json-ref#" ||
             identifier == "http://json-schema.org/draft-00/json-ref") {
    promise.set_value(sourcemeta::jsontoolkit::parse(
        R"EOF(@METASCHEMA_JSON_REF_DRAFT0@)EOF"));

    // Otherwise
  } else {
    promise.set_value(std::nullopt);
  }

  return promise.get_future();
}
