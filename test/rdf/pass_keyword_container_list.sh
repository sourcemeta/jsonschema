#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
      "steps": {
        "type": "array",
        "x-jsonld-id": "https://schema.org/step",
        "x-jsonld-container": "@list"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "steps": [ 1, 2, 3 ] }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/step": [
      {
        "@list": [
          {
            "@value": 1
          },
          {
            "@value": 2
          },
          {
            "@value": 3
          }
        ]
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
