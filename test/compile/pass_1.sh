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
    "category": "assertion",
    "type": "property-type-strict",
    "value": {
      "category": "value",
      "type": "type",
      "value": "string"
    },
    "schemaResource": 0,
    "absoluteKeywordLocation": "#/properties/foo/type",
    "relativeSchemaLocation": "/properties/foo/type",
    "relativeInstanceLocation": "/foo",
    "dynamic": false,
    "track": false
  }
]
EOF

diff "$TMP/result.json" "$TMP/expected.json"
