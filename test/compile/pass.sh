#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "properties": {
    "foo": { "type": "string" }
  }
}
EOF

"$1" compile "$TMP/schema.json" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "category": "logical",
    "type": "and",
    "value": null,
    "absoluteKeywordLocation": "#/properties",
    "relativeSchemaLocation": "/properties",
    "relativeInstanceLocation": "",
    "target": {
      "category": "target",
      "type": "instance",
      "location": ""
    },
    "condition": [
      {
        "category": "assertion",
        "type": "type-strict",
        "value": {
          "category": "value",
          "type": "type",
          "value": "object"
        },
        "absoluteKeywordLocation": "#/properties",
        "relativeSchemaLocation": "",
        "relativeInstanceLocation": "",
        "target": {
          "category": "target",
          "type": "instance",
          "location": ""
        },
        "condition": []
      }
    ],
    "children": [
      {
        "category": "internal",
        "type": "container",
        "value": null,
        "absoluteKeywordLocation": "#/properties",
        "relativeSchemaLocation": "",
        "relativeInstanceLocation": "",
        "target": {
          "category": "target",
          "type": "instance",
          "location": ""
        },
        "condition": [
          {
            "category": "assertion",
            "type": "defines",
            "value": {
              "category": "value",
              "type": "string",
              "value": "foo"
            },
            "absoluteKeywordLocation": "#/properties",
            "relativeSchemaLocation": "",
            "relativeInstanceLocation": "",
            "target": {
              "category": "target",
              "type": "instance",
              "location": ""
            },
            "condition": []
          }
        ],
        "children": [
          {
            "category": "assertion",
            "type": "type-strict",
            "value": {
              "category": "value",
              "type": "type",
              "value": "string"
            },
            "absoluteKeywordLocation": "#/properties/foo/type",
            "relativeSchemaLocation": "/foo/type",
            "relativeInstanceLocation": "/foo",
            "target": {
              "category": "target",
              "type": "instance",
              "location": ""
            },
            "condition": []
          },
          {
            "category": "annotation",
            "type": "public",
            "value": {
              "category": "value",
              "type": "json",
              "value": "foo"
            },
            "absoluteKeywordLocation": "#/properties",
            "relativeSchemaLocation": "",
            "relativeInstanceLocation": "",
            "target": {
              "category": "target",
              "type": "instance",
              "location": ""
            },
            "condition": []
          }
        ]
      }
    ]
  }
]
EOF

diff "$TMP/result.json" "$TMP/expected.json"
