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
  "x-jsonld-type": "https://schema.org/Person",
  "properties": {
    "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "Ada" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" --flatten > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "@id": "_:b0",
    "@type": [ "https://schema.org/Person" ],
    "https://schema.org/name": [
      {
        "@value": "Ada"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" -l > "$TMP/output.json"

diff "$TMP/output.json" "$TMP/expected.json"
