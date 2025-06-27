#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
$schema: https://json-schema.org/draft/2020-12/schema
$id: https://example.com
$ref: '#/$defs/string'
$defs:
  string: { type: string }
EOF

"$1" compile "$TMP/schema.yaml" > "$TMP/template.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "dynamic": false,
  "track": true,
  "instructions": [
    {
      "t": 51,
      "s": "/$ref",
      "i": "",
      "k": "https://example.com#/$ref",
      "r": 1,
      "v": {
        "t": 0,
        "v": null
      },
      "c": [
        {
          "t": 11,
          "s": "/type",
          "i": "",
          "k": "https://example.com#/$defs/string/type",
          "r": 1,
          "v": {
            "t": 8,
            "v": 4
          },
          "c": []
        }
      ]
    }
  ]
}
EOF

diff "$TMP/template.json" "$TMP/expected.json"
