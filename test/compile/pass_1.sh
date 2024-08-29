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
    "schemaResource": "",
    "absoluteKeywordLocation": "#/properties",
    "relativeSchemaLocation": "/properties",
    "relativeInstanceLocation": "",
    "target": {
      "category": "target",
      "type": "instance",
      "location": ""
    },
    "report": true,
    "dynamic": false,
    "condition": [],
    "children": [
      {
        "category": "logical",
        "type": "and",
        "value": null,
        "schemaResource": "",
        "absoluteKeywordLocation": "#/properties",
        "relativeSchemaLocation": "",
        "relativeInstanceLocation": "",
        "target": {
          "category": "target",
          "type": "instance",
          "location": ""
        },
        "report": false,
        "dynamic": false,
        "condition": [
          {
            "category": "assertion",
            "type": "defines",
            "value": {
              "category": "value",
              "type": "string",
              "value": "foo"
            },
            "schemaResource": "",
            "absoluteKeywordLocation": "#/properties",
            "relativeSchemaLocation": "",
            "relativeInstanceLocation": "",
            "target": {
              "category": "target",
              "type": "instance",
              "location": ""
            },
            "report": false,
            "dynamic": false,
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
            "schemaResource": "",
            "absoluteKeywordLocation": "#/properties/foo/type",
            "relativeSchemaLocation": "/foo/type",
            "relativeInstanceLocation": "/foo",
            "target": {
              "category": "target",
              "type": "instance",
              "location": ""
            },
            "report": true,
            "dynamic": false,
            "condition": []
          }
        ]
      }
    ]
  }
]
EOF

diff "$TMP/result.json" "$TMP/expected.json"
