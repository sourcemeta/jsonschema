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
      "value": {
        "oneOf": [
          { "type": "integer", "x-jsonld-id": "https://schema.org/age" },
          { "type": "string", "x-jsonld-id": "https://schema.org/name" }
        ]
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "value": "Ada" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/name": [
      {
        "@value": "Ada"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
