#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "foo": { "type": "string" }
  }
}
EOF

cat << 'EOF' > "$TMP/original.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "foo": { "type": "string" }
  }
}
EOF

"$1" canonicalize "$TMP/schema.json" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "anyOf": [
    {
      "enum": [ null ]
    },
    {
      "enum": [ false, true ]
    },
    {
      "type": "object",
      "minProperties": 0,
      "properties": {
        "foo": {
          "type": "string",
          "minLength": 0
        }
      }
    },
    {
      "type": "array",
      "minItems": 0
    },
    {
      "type": "string",
      "minLength": 0
    },
    {
      "type": "number"
    }
  ]
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"

diff "$TMP/schema.json" "$TMP/original.json"
