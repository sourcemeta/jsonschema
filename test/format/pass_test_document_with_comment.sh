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
      "valid": true,
      "data": {}
    }
  ],
  "$comment": "Some test comment",
  "target": "https://example.com/my-schema"
}
EOF

"$1" fmt "$TMP/test.json" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected_output.txt"
Interpreting as a test file: $(realpath "$TMP")/test.json
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$comment": "Some test comment",
  "target": "https://example.com/my-schema",
  "tests": [
    {
      "valid": true,
      "data": {}
    }
  ]
}
EOF

diff "$TMP/test.json" "$TMP/expected.json"
