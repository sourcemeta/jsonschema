#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "tests": [],
  "target": "https://example.com/my-schema"
}
EOF

"$1" fmt "$TMP/test.json" \
  --default-dialect "https://json-schema.org/draft/2020-12/schema" \
  > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "target": "https://example.com/my-schema",
  "tests": []
}
EOF

diff "$TMP/test.json" "$TMP/expected.json"
