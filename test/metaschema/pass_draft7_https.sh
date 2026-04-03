#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

"$1" metaschema "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
