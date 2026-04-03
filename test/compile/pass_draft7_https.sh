#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
  "additionalProperties": {
    "type": "string"
  }
}
EOF

"$1" compile "$TMP/schema.json" > "$TMP/template.json" 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected_stderr.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"

cat << 'EOF' > "$TMP/expected.json"
[
  1,
  false,
  true,
  [
    [
      [
        61,
        [ "additionalProperties" ],
        [],
        "https://example.com#/additionalProperties",
        2,
        [ 0 ],
        [
          [
            11,
            [ "type" ],
            [],
            "https://example.com#/additionalProperties/type",
            2,
            [ 8, 4 ]
          ]
        ]
      ]
    ]
  ],
  []
]
EOF

diff "$TMP/template.json" "$TMP/expected.json"
