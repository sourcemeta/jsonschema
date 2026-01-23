#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "additionalProperties": {
    "type": "string"
  }
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cd "$TMP"

"$1" compile schema.json --verbose > "$TMP/template.json" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
[
  false,
  true,
  [ "", "https://example.com" ],
  [
    [
      [
        61,
        "/additionalProperties",
        "",
        "#/additionalProperties",
        2,
        [ 0 ],
        [
          [
            11,
            "/type",
            "",
            "#/additionalProperties/type",
            2,
            [ 8, 4 ]
          ],
          [
            46,
            "",
            "",
            "#/additionalProperties",
            2,
            [ 0 ]
          ]
        ]
      ]
    ]
  ],
  []
]
EOF

diff "$TMP/template.json" "$TMP/expected.json"

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
