#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object",
  "properties": {
    "name": {
      "type": "string"
    }
  }
}
EOF

mkdir "$TMP/instances"

cat << 'EOF' > "$TMP/instances/instance_1.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/instances/instance_2.json"
{ "name": "Bob" }
EOF

cat << 'EOF' > "$TMP/single.json"
{ "name": "Charlie" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/single.json" "$TMP/instances" 2> "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
