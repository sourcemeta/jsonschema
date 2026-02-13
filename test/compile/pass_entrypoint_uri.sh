#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com/schema",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "$defs": {
    "Name": {
      "type": "string",
      "minLength": 1
    }
  }
}
EOF

"$1" compile "$TMP/schema.json" \
  --entrypoint "https://example.com/schema#/\$defs/Name" > "$TMP/output.json" 2>&1

cat << 'EOF' > "$TMP/expected.json"
[
  false,
  true,
  [ "", "https://example.com/schema" ],
  [
    [
      [
        21,
        "/minLength",
        "",
        "#/$defs/Name/minLength",
        2,
        [ 10, 0 ]
      ],
      [
        11,
        "/type",
        "",
        "#/$defs/Name/type",
        2,
        [ 8, 4 ]
      ]
    ]
  ],
  []
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
