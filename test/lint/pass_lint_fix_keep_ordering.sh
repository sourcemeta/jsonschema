#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "examples": ["foo"],
  "$schema": "http://json-schema.org/draft-06/schema#"
}
EOF

"$1" lint "$TMP/schema.json" --fix > "$TMP/result.txt" 2>&1

cat << 'EOF' > "$TMP/output.txt"
EOF

diff "$TMP/result.txt" "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "examples": ["foo"],
  "$schema": "http://json-schema.org/draft-06/schema#"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
