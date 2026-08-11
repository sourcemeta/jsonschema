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
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/data-valid.json"
"Hello World"
EOF

cat << 'EOF' > "$TMP/data-invalid.json"
{ "type": "Hello World" }
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "dataPath": "./data-valid.json"
    },
    {
      "description": "Second test",
      "valid": false,
      "dataPath": "./data-invalid.json"
    }
  ]
}
EOF

cd "$TMP"

# A test document read from standard input has no directory of its own, so
# its relative data paths resolve against the working directory
"$1" test - --resolve "$TMP/schema.json" < "$TMP/test.json" \
  1> "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
tag:sourcemeta.com,2026:jsonschema/stdin: PASS 2/2
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
