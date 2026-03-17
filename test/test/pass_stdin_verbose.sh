#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' | "$1" test - --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "A string",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "Not a string",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/expected.txt"
/dev/stdin:
  1/2 PASS A string
  2/2 PASS Not a string
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
