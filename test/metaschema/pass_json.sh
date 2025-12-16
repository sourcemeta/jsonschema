#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
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
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "$schema", "$defs" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "Core vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/1/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/applicator#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "Applicator vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/2/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/unevaluated#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "Unevaluated applicator vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/3/$ref/properties",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "type" ]
    },
    {
      "keywordLocation": "/allOf/3/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "Validation vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/4/$ref/properties",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/meta-data#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "title", "description" ]
    },
    {
      "keywordLocation": "/allOf/4/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/meta-data#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "Meta-data vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/5/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/format-annotation#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "Format vocabulary meta-schema for annotation results" ]
    },
    {
      "keywordLocation": "/allOf/6/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/content#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "Content vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/schema#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 11, 1 ],
      "annotation": [ "Core and Validation specifications meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/properties/$defs/additionalProperties",
      "instanceLocation": "/$defs",
      "instancePosition": [ 6, 3, 10, 3 ],
      "annotation": [ "foo" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/0/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/title",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "Core vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/1/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/applicator#/title",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "Applicator vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/2/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/unevaluated#/title",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "Unevaluated applicator vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/3/$ref/properties",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/properties",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "type" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/3/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/title",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "Validation vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/4/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/meta-data#/title",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "Meta-data vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/5/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/format-annotation#/title",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "Format vocabulary meta-schema for annotation results" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/6/$ref/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/content#/title",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "Content vocabulary meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/title",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/schema#/title",
      "instanceLocation": "/$defs/foo",
      "instancePosition": [ 7, 5, 9, 5 ],
      "annotation": [ "Core and Validation specifications meta-schema" ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$defs/additionalProperties/$dynamicRef/allOf/3/$ref/properties/type/anyOf/1/items",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/validation#/properties/type/anyOf/1/items",
      "instanceLocation": "/$defs/foo/type",
      "instancePosition": [ 8, 7, 8, 34 ],
      "annotation": [ true ]
    },
    {
      "keywordLocation": "/allOf/0/$ref/properties/$schema/$ref/format",
      "absoluteKeywordLocation": "https://json-schema.org/draft/2020-12/meta/core#/$defs/uriString/format",
      "instanceLocation": "/$schema",
      "instancePosition": [ 2, 3, 2, 59 ],
      "annotation": [ "uri" ]
    }
  ]
}
EOF

diff "$TMP/expected.json" "$TMP/output.json"
