#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cassert> // assert

static auto quantify(const std::int64_t value, const std::string &singular,
                     const std::string &plural) -> std::string {
  std::ostringstream result;
  result << value;
  result << ' ';
  result << (value == 1 ? singular : plural);
  return result.str();
}

static auto
make_range_constraint(std::map<std::string, std::string> &constraints,
                      const std::optional<std::int64_t> &minimum,
                      const std::optional<std::int64_t> &maximum,
                      const std::optional<std::int64_t> &lower_bound,
                      const std::string &singular,
                      const std::string &plural) -> void {
  if (constraints.contains("range")) {
    return;
  } else if ((!minimum.has_value() ||
              (lower_bound.has_value() &&
               minimum.value() <= lower_bound.value())) &&
             maximum.has_value()) {
    constraints.emplace("range",
                        "<= " + quantify(maximum.value(), singular, plural));
  } else if (minimum.has_value() && maximum.has_value()) {
    if (minimum.value() == maximum.value()) {
      constraints.emplace(
          "range", "exactly " + quantify(minimum.value(), singular, plural));
    } else {
      constraints.emplace("range",
                          std::to_string(minimum.value()) + " to " +
                              quantify(maximum.value(), singular, plural));
    }
  } else if (minimum.has_value()) {
    if (lower_bound.has_value() && minimum.value() <= lower_bound.value()) {
      return;
    }

    constraints.emplace("range",
                        ">= " + quantify(minimum.value(), singular, plural));
  }
}

static auto
explain_constant_from_value(const sourcemeta::jsontoolkit::JSON &schema,
                            const sourcemeta::jsontoolkit::JSON &value)
    -> std::optional<sourcemeta::jsontoolkit::SchemaExplanation> {
  std::optional<std::string> title;
  std::optional<std::string> description;

  if (schema.defines("title")) {
    assert(schema.at("title").is_string());
    title = schema.at("title").to_string();
  } else if (schema.defines("description")) {
    assert(schema.at("description").is_string());
    description = schema.at("description").to_string();
  }

  return sourcemeta::jsontoolkit::SchemaExplanationConstant{value, title,
                                                            description};
}

static auto translate_format(const std::string &type) -> std::string {
  if (type == "date-time") {
    return "Timestamp";
  } else if (type == "date") {
    return "Date";
  } else if (type == "time") {
    return "Time";
  } else if (type == "email") {
    return "Email Address";
  } else if (type == "idn-email") {
    return "Email Address";
  } else if (type == "hostname") {
    return "Hostname";
  } else if (type == "idn-hostname") {
    return "Hostname";
  } else if (type == "ipv4") {
    return "IP Address v4";
  } else if (type == "ipv6") {
    return "IP Address v6";
  } else if (type == "uri") {
    return "Absolute URI";
  } else if (type == "uri-reference") {
    return "URI";
  } else if (type == "iri") {
    return "Absolute URI";
  } else if (type == "iri-reference") {
    return "URI";
  } else if (type == "uri-template") {
    return "URI Template";
  } else if (type == "json-pointer") {
    return "JSON Pointer";
  } else if (type == "relative-json-pointer") {
    return "Relative JSON Pointer";
  } else if (type == "regex") {
    return "Regular Expression";
  } else {
    return type;
  }
}

static auto explain_string(const sourcemeta::jsontoolkit::JSON &schema,
                           const std::map<std::string, bool> &vocabularies)
    -> std::optional<sourcemeta::jsontoolkit::SchemaExplanation> {
  sourcemeta::jsontoolkit::SchemaExplanationScalar explanation;
  explanation.type = "String";

  if (vocabularies.contains("http://json-schema.org/draft-07/schema#")) {
    if (schema.defines("minLength") && schema.defines("maxLength")) {
      make_range_constraint(
          explanation.constraints, schema.at("minLength").to_integer(),
          schema.at("maxLength").to_integer(), 0, "character", "characters");
    }

    static const std::set<std::string> IGNORE{"$id", "$schema", "$comment",
                                              "type"};
    for (const auto &[keyword, value] : schema.as_object()) {
      if (IGNORE.contains(keyword)) {
        continue;
      } else if (keyword == "title") {
        assert(value.is_string());
        explanation.title = value.to_string();
      } else if (keyword == "description") {
        assert(value.is_string());
        explanation.description = value.to_string();
      } else if (keyword == "minLength") {
        make_range_constraint(explanation.constraints,
                              schema.at("minLength").to_integer(), std::nullopt,
                              0, "character", "characters");
      } else if (keyword == "maxLength") {
        make_range_constraint(explanation.constraints, std::nullopt,
                              schema.at("maxLength").to_integer(), 0,
                              "character", "characters");
      } else if (keyword == "pattern") {
        assert(value.is_string());
        explanation.constraints.emplace("matches", value.to_string());
      } else if (keyword == "format") {
        assert(value.is_string());
        explanation.type =
            "String (" + translate_format(value.to_string()) + ")";
      } else if (keyword == "examples") {
        assert(value.is_array());
        for (const auto &item : value.as_array()) {
          assert(item.is_string());
          explanation.examples.insert(item);
        }
      } else {
        return std::nullopt;
      }
    }
  }

  return explanation;
}

static auto explain_enumeration(const sourcemeta::jsontoolkit::JSON &schema,
                                const std::map<std::string, bool> &vocabularies)
    -> std::optional<sourcemeta::jsontoolkit::SchemaExplanation> {
  sourcemeta::jsontoolkit::SchemaExplanationEnumeration explanation;
  if (vocabularies.contains("http://json-schema.org/draft-07/schema#")) {
    static const std::set<std::string> IGNORE{
        "$id", "$schema", "$comment",
        // We don't collect examples, as by definition
        // the enumeration is a set of examples
        "examples"};
    for (const auto &[keyword, value] : schema.as_object()) {
      if (IGNORE.contains(keyword)) {
        continue;
      } else if (keyword == "enum") {
        assert(value.is_array());
        for (const auto &item : value.as_array()) {
          explanation.values.insert(item);
        }
      } else if (keyword == "title") {
        assert(value.is_string());
        explanation.title = value.to_string();
      } else if (keyword == "description") {
        assert(value.is_string());
        explanation.description = value.to_string();
      } else {
        return std::nullopt;
      }
    }
  }

  return explanation;
}

namespace sourcemeta::jsontoolkit {

auto explain(const JSON &schema, const SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect)
    -> std::optional<SchemaExplanation> {
  assert(is_schema(schema));

  if (schema.is_boolean()) {
    return std::nullopt;
  }

  const auto vocabularies{
      sourcemeta::jsontoolkit::vocabularies(schema, resolver, default_dialect)
          .get()};

  // TODO: Support all dialects

  if (vocabularies.contains("http://json-schema.org/draft-07/schema#") &&
      schema.defines("type") && schema.at("type").is_string() &&
      schema.at("type").to_string() == "string") {

    if (schema.defines("maxLength") && schema.at("maxLength").is_integer() &&
        schema.at("maxLength").to_integer() == 0) {
      return explain_constant_from_value(schema,
                                         sourcemeta::jsontoolkit::JSON{""});
    }

    return explain_string(schema, vocabularies);
  }

  if (vocabularies.contains("http://json-schema.org/draft-07/schema#") &&
      schema.defines("enum") && schema.at("enum").is_array() &&
      !schema.at("enum").empty()) {
    return explain_enumeration(schema, vocabularies);
  }

  return std::nullopt;
}

} // namespace sourcemeta::jsontoolkit
