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
      "description": "Wrong expectation",
      "valid": true,
      "data": { "name": "Ada" },
      "rdf": [
        {
          "@type": [ "https://schema.org/Person" ],
          "https://schema.org/name": [ { "@value": "Grace" } ]
        }
      ]
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  1/1 FAIL Wrong expectation

error: RDF expansion mismatch
  expected:
    [
      {
        "@type": [ "https://schema.org/Person" ],
        "https://schema.org/name": [
          {
            "@value": "Grace"
          }
        ]
      }
    ]
  but got:
    [
      {
        "@type": [ "https://schema.org/Person" ],
        "https://schema.org/name": [
          {
            "@value": "Ada"
          }
        ]
      }
    ]
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
