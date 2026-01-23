#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com",
  "$ref": "other"
}
EOF

cat << 'EOF' > "$TMP/remote.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/nested",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "resolve": {
    "https://example.com/other": "https://example.com/middle",
    "https://example.com/middle": "https://example.com/nested"
  }
}
EOF

cd "$TMP"

"$1" compile schema.json \
  --resolve remote.json --verbose > "$TMP/template.json" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
[
  false,
  true,
  [ "", "https://example.com", "https://example.com/other" ],
  [
    [
      [
        91,
        "/$ref",
        "",
        "#/$ref",
        2,
        [ 10, 1 ]
      ],
      [
        44,
        "/description",
        "",
        "#/description",
        2,
        [ 1, "Test schema" ]
      ],
      [
        44,
        "/title",
        "",
        "#/title",
        2,
        [ 1, "Test" ]
      ]
    ],
    [
      [
        44,
        "/description",
        "",
        "#/description",
        3,
        [ 1, "Test schema" ]
      ],
      [
        44,
        "/title",
        "",
        "#/title",
        3,
        [ 1, "Test" ]
      ],
      [
        11,
        "/type",
        "",
        "#/type",
        3,
        [ 8, 4 ]
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
