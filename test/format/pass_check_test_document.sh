#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com/my-schema",
  "tests": [
    {
      "description": "I expect to pass",
      "valid": true,
      "data": {
        "bar": 1,
        "foo": 1
      }
    }
  ]
}
EOF

"$1" fmt "$TMP/test.json" --check > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"
