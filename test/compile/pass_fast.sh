#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "additionalProperties": {
    "type": "string"
  }
}
EOF

"$1" compile "$TMP/schema.json" --fast > "$TMP/template.json"

cat << 'EOF' > "$TMP/expected.json"
[
  false,
  false,
  [ "", "https://example.com" ],
  [
    [
      71,
      "/additionalProperties",
      "",
      "#/additionalProperties",
      2,
      [ 8, 4 ]
    ]
  ]
]
EOF

diff "$TMP/template.json" "$TMP/expected.json"
