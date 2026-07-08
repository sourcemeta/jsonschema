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
      "data": {
        "type": "object",
        "x-jsonld-id": "https://schema.org/data",
        "x-jsonld-container": "@index"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "data": { "a": "foo", "b": "bar" } }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/data": [
      {
        "@value": "foo"
      },
      {
        "@value": "bar"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
