#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/schemas"
mkdir -p "$TMP/schemas/ignored"

cat << 'EOF' > "$TMP/schemas/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schemas/ignored/bad.json"
{
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./schemas",
  "ignore": [
    "./schemas/ignored"
  ]
}
EOF

cd "$TMP"
"$1" metaschema > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
