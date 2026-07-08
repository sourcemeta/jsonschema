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
      "currency": {
        "type": "string",
        "x-jsonld-id": "https://schema.org/priceCurrency",
        "x-jsonld-type": "https://schema.org/Currency",
        "x-jsonld-self": "https://www.iso.org/iso-4217/{this}"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "currency": "USD" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/priceCurrency": [
      {
        "@id": "https://www.iso.org/iso-4217/USD",
        "@type": [ "https://schema.org/Currency" ]
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
