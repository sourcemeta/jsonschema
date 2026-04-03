#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft/2019-09/schema",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "$defs": {
    "foo": {
      "type": [ "string", "null" ]
    }
  }
}
EOF

"$1" metaschema "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
