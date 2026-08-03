#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/first.json"
{
  "$id": "https://example.com/first",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "x-jsonld-type": "https://schema.org/Person",
  "properties": {
    "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
  }
}
EOF

cat << 'EOF' > "$TMP/schemas/second.json"
{
  "$id": "https://example.com/second",
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
  "target": [ "https://example.com/first", "https://example.com/second" ],
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
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schemas" 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json: PASS 2/2
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
