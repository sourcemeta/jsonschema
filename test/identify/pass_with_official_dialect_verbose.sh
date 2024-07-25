#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

"$1" identify --verbose "$TMP/schema.json" > "$TMP/result.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
https://example.com
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
