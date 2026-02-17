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
  "type": "object",
  "properties": {
    "name": {
      "type": "string"
    }
  }
}
EOF

mkdir -p "$TMP/instances"
mkdir -p "$TMP/instances/drafts"

cat << 'EOF' > "$TMP/instances/valid.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/instances/drafts/invalid.json"
{ "name": 123 }
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "ignore": [
    "./instances/drafts"
  ]
}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
