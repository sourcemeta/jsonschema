#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-03/schema#",
  "id": "https://example.com/test",
  "type": "object",
  "properties": {
    "name": {
      "type": "string",
      "required": true
    },
    "age": {
      "type": "integer",
      "divisibleBy": 1,
      "disallow": "string"
    }
  },
  "extends": {
    "type": "object"
  }
}
EOF

"$1" upgrade --to 2020-12 "$TMP/schema.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test",
  "type": "object",
  "allOf": [
    {
      "type": "object"
    }
  ],
  "required": [ "name" ],
  "properties": {
    "name": {
      "type": "string"
    },
    "age": {
      "type": "integer",
      "not": {
        "type": "string"
      },
      "multipleOf": 1
    }
  }
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
