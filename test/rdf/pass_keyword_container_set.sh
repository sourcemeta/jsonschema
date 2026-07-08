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
      "tags": {
        "type": "array",
        "x-jsonld-id": "https://schema.org/keywords",
        "x-jsonld-container": "@set"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "tags": [ "a", "b" ] }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/keywords": [
      {
        "@value": "a"
      },
      {
        "@value": "b"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
