#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/foo/bar/baz",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

"$1" identify "$TMP/schema.json" \
  --relative-from "https://example.com/foo" > "$TMP/result.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
/bar/baz
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
