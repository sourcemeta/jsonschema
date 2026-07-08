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
      "parent": {
        "type": "object",
        "x-jsonld-reverse": "https://schema.org/children",
        "x-jsonld-type": "https://schema.org/Person"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "parent": {} }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "@reverse": {
      "https://schema.org/children": [
        {
          "@type": [ "https://schema.org/Person" ]
        }
      ]
    }
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
