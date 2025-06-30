#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "$defs": {
    "foo": {
      "type": [ "string", "null" ]
    }
  }
}
EOF

"$1" metaschema "$TMP/schema.json" --json > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "valid": true,
  "annotations": [
    {
      "keywordLocation": "/allOf/0/$ref/properties",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/properties",
      "instanceLocation": "",
      "annotation": [ "$schema", "$defs" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/title",
      "instanceLocation": "",
      "annotation": [ "Core vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/1/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/applicator#/title",
      "instanceLocation": "",
      "annotation": [ "Applicator vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/2/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/unevaluated#/title",
      "instanceLocation": "",
      "annotation": [ "Unevaluated applicator vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/3/$ref/properties",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/properties",
      "instanceLocation": "",
      "annotation": [ "type" ]
    },
    {
      "keywordLocation": "/allOf/3/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/title",
      "instanceLocation": "",
      "annotation": [ "Validation vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/4/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/meta-data#/title",
      "instanceLocation": "",
      "annotation": [ "Meta-data vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/5/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/format-annotation#/title",
      "instanceLocation": "",
      "annotation": [ "Format vocabulary meta-schema for annotation results" ]
    },
    {
      "keywordLocation": "/allOf/6/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/content#/title",
      "instanceLocation": "",
      "annotation": [ "Content vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/schema#/title",
      "instanceLocation": "",
      "annotation": [ "Core and Validation specifications meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/properties/$defs/additionalProperties",
      "instanceLocation": "/$defs",
      "annotation": [ "foo" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/0/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/title",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "Core vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/1/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/applicator#/title",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "Applicator vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/2/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/unevaluated#/title",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "Unevaluated applicator vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/3/$ref/properties",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/properties",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "type" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/3/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/title",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "Validation vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/4/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/meta-data#/title",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "Meta-data vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/5/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/format-annotation#/title",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "Format vocabulary meta-schema for annotation results" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/6/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/content#/title",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "Content vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/schema#/title",
      "instanceLocation": "/$defs/foo",
      "annotation": [ "Core and Validation specifications meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/3/$ref/properties/type/anyOf/1/items",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/properties/type/anyOf/1/items",
      "instanceLocation": "/$defs/foo/type",
      "annotation": [ true ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$schema/$ref/format",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/$defs/uriString/format",
      "instanceLocation": "/$schema",
      "annotation": [ "uri" ]
    }
  ]
}
EOF

diff "$TMP/expected.json" "$TMP/output.json"
