#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

"$1" metaschema "$TMP/schema.json"
