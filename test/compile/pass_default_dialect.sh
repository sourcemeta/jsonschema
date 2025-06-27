#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "additionalProperties": {
    "type": "string"
  }
}
EOF

"$1" compile "$TMP/schema.json" \
  --default-dialect "https://json-schema.org/draft/2020-12/schema" > "$TMP/template.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "dynamic": false,
  "track": true,
  "instructions": [
    {
      "t": 61,
      "s": "/additionalProperties",
      "i": "",
      "k": "#/additionalProperties",
      "r": 0,
      "v": {
        "t": 0,
        "v": null
      },
      "c": [
        {
          "t": 11,
          "s": "/type",
          "i": "",
          "k": "#/additionalProperties/type",
          "r": 0,
          "v": {
            "t": 8,
            "v": 4
          },
          "c": []
        },
        {
          "t": 46,
          "s": "",
          "i": "",
          "k": "#/additionalProperties",
          "r": 0,
          "v": {
            "t": 0,
            "v": null
          },
          "c": []
        }
      ]
    }
  ]
}
EOF

diff "$TMP/template.json" "$TMP/expected.json"
