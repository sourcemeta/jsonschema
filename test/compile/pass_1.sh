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
    "category": "loop",
    "type": "properties-match",
    "value": {
      "category": "value",
      "type": "named-indexes",
      "value": {
        "foo": 0
      }
    },
    "schemaResource": "",
    "absoluteKeywordLocation": "#/properties",
    "relativeSchemaLocation": "/properties",
    "relativeInstanceLocation": "",
    "report": true,
    "dynamic": false,
    "children": [
      {
        "category": "logical",
        "type": "and",
        "value": null,
        "schemaResource": "",
        "absoluteKeywordLocation": "#/properties",
        "relativeSchemaLocation": "",
        "relativeInstanceLocation": "",
        "report": false,
        "dynamic": false,
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
            "report": true,
            "dynamic": false
          }
        ]
      }
    ]
  }
]
EOF

diff "$TMP/result.json" "$TMP/expected.json"
