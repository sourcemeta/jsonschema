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
      "dob": {
        "type": "string",
        "x-jsonld-id": "https://schema.org/birthDate",
        "x-jsonld-datatype": "http://www.w3.org/2001/XMLSchema#date"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "dob": "1990-05-15" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/birthDate": [
      {
        "@value": "1990-05-15",
        "@type": "http://www.w3.org/2001/XMLSchema#date"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
