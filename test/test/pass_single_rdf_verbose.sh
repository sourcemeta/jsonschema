#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "x-jsonld-type": "https://schema.org/Person",
  "properties": {
    "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
  }
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": { "name": "Ada" },
      "rdf": [
        {
          "@type": [ "https://schema.org/Person" ],
          "https://schema.org/name": [ { "@value": "Ada" } ]
        }
      ]
    },
    {
      "description": "Invalid type",
      "valid": false,
      "data": { "name": 1 }
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  1/2 PASS First test
  2/2 PASS Invalid type
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
