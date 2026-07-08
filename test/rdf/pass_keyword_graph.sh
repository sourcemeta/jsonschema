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
    "x-jsonld-graph": true,
    "properties": {
      "member": {
        "type": "object",
        "x-jsonld-id": "https://schema.org/member",
        "x-jsonld-type": "https://schema.org/Person"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "member": {} }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "@graph": [
      {
        "https://schema.org/member": [
          {
            "@type": [ "https://schema.org/Person" ]
          }
        ]
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
