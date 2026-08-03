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
    "x-jsonld-type": "https://schema.org/Product",
    "x-jsonld-constants": {
      "https://schema.org/brand": { "@id": "https://example.com/brands/acme" },
      "https://schema.org/category": "electronics"
    },
    "properties": {
      "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "Vacuum Robot" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "@type": [ "https://schema.org/Product" ],
    "https://schema.org/name": [
      {
        "@value": "Vacuum Robot"
      }
    ],
    "https://schema.org/brand": [
      {
        "@id": "https://example.com/brands/acme"
      }
    ],
    "https://schema.org/category": [
      {
        "@value": "electronics"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
