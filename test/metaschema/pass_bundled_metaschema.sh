#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/meta",
  "$id": "https://example.com/schema",
  "type": "string",
  "$defs": {
    "https://example.com/meta": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/meta",
      "$vocabulary": {
        "https://json-schema.org/draft/2020-12/vocab/core": true,
        "https://json-schema.org/draft/2020-12/vocab/validation": true
      },
      "type": "object"
    }
  }
}
EOF

"$1" metaschema "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
