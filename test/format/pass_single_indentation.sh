#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "description": "Test schema",
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

"$1" fmt "$TMP/schema.json" --indentation 4 >"$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "title": "Hello World",
    "description": "Test schema",
    "properties": {
        "foo": {},
        "bar": {}
    },
    "additionalProperties": false
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
