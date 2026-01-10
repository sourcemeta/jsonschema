#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

SCHEMA_PATH="$(cd "$TMP" && pwd)/schema.json"
SCHEMA_URI="file://$SCHEMA_PATH"

"$1" metaschema "$SCHEMA_URI" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

