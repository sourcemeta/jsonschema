#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": { "foo": {} },
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "tests": [],
  "target": "https://example.com/my-schema"
}
EOF

"$1" fmt "$TMP" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected_output.txt"
Interpreting as a test file: $(realpath "$TMP")/test.json
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected_test.json"
{
  "target": "https://example.com/my-schema",
  "tests": []
}
EOF

diff "$TMP/test.json" "$TMP/expected_test.json"

cat << 'EOF' > "$TMP/expected_schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "foo": {}
  }
}
EOF

diff "$TMP/schema.json" "$TMP/expected_schema.json"
