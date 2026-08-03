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
      "weight": {
        "type": "number",
        "x-jsonld-id": "https://schema.org/weight",
        "x-jsonld-value": "https://schema.org/value",
        "x-jsonld-type": "https://schema.org/QuantitativeValue",
        "x-jsonld-constants": {
          "https://schema.org/unitCode": "KGM"
        }
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "weight": 2.5 }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/weight": [
      {
        "@type": [ "https://schema.org/QuantitativeValue" ],
        "https://schema.org/value": [
          {
            "@value": 2.5
          }
        ],
        "https://schema.org/unitCode": [
          {
            "@value": "KGM"
          }
        ]
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
