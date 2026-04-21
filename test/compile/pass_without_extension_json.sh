#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

"$1" compile "$TMP/schema" > "$TMP/template.json"

cat << 'EOF' > "$TMP/expected.json"
[
  4,
  false,
  true,
  [
    [
      [
        98,
        [ "$ref" ],
        [],
        "https://example.com#/$ref",
        2,
        [ 10, 1 ]
      ]
    ],
    [
      [
        11,
        [ "type" ],
        [],
        "https://example.com#/$defs/string/type",
        2,
        [ 8, 4 ]
      ]
    ]
  ],
  []
]
EOF

diff "$TMP/template.json" "$TMP/expected.json"
