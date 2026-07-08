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
      "text": {
        "type": "string",
        "x-jsonld-id": "https://schema.org/text",
        "x-jsonld-direction": "rtl"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "text": "abc" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/text": [
      {
        "@value": "abc",
        "@direction": "rtl"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
