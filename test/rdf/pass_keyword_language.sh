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
      "greeting": {
        "type": "string",
        "x-jsonld-id": "https://schema.org/name",
        "x-jsonld-language": "en"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "greeting": "hello" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/name": [
      {
        "@value": "hello",
        "@language": "en"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
