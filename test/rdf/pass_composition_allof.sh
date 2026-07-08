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
    "allOf": [
      { "x-jsonld-type": "https://schema.org/Person" },
      { "x-jsonld-type": "https://schema.org/Agent" }
    ],
    "properties": {
      "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "Ada" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "@type": [ "https://schema.org/Agent", "https://schema.org/Person" ],
    "https://schema.org/name": [
      {
        "@value": "Ada"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
