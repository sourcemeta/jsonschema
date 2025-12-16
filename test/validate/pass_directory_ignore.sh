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

mkdir -p "$TMP/instances/drafts"

cat << 'EOF' > "$TMP/instances/instance_1.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/instances/drafts/invalid.json"
{ "name": 123 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --ignore "$TMP/instances/drafts" 2> "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
