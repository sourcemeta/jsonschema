#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "tests": [
    {
      "description": "I expect to pass",
      "valid": true,
      "data": { "foo": 1 }
    }
  ],
  "target": "https://example.com/my-schema"
}
EOF

"$1" fmt - < "$TMP/test.json" > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
Interpreting as a test file: /dev/stdin
{
  "target": "https://example.com/my-schema",
  "tests": [
    {
      "description": "I expect to pass",
      "valid": true,
      "data": {
        "foo": 1
      }
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
