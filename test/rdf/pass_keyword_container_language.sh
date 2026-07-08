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
      "label": {
        "type": "object",
        "x-jsonld-id": "https://schema.org/name",
        "x-jsonld-container": "@language"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "label": { "en": "Hello", "fr": "Bonjour" } }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/name": [
      {
        "@value": "Hello",
        "@language": "en"
      },
      {
        "@value": "Bonjour",
        "@language": "fr"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
