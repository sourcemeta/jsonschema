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
  "title": "Test",
  "description": "Test schema",
  "additionalProperties": {
    "type": "string"
  }
}
EOF

"$1" compile "$TMP/schema.json" > "$TMP/template.json"

cat << 'EOF' > "$TMP/expected.json"
[
  1,
  false,
  true,
  [
    [
      [
        44,
        [ "description" ],
        [],
        "https://example.com#/description",
        2,
        [ 1, "Test schema" ]
      ],
      [
        44,
        [ "title" ],
        [],
        "https://example.com#/title",
        2,
        [ 1, "Test" ]
      ],
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
          ],
          [
            46,
            [],
            [],
            "https://example.com#/additionalProperties",
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
