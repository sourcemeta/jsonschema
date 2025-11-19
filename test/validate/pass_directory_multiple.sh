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

mkdir "$TMP/dir1"
mkdir "$TMP/dir2"

cat << 'EOF' > "$TMP/dir1/instance_1.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/dir1/instance_2.json"
{ "name": "Bob" }
EOF

cat << 'EOF' > "$TMP/dir2/instance_3.json"
{ "name": "Charlie" }
EOF

cat << 'EOF' > "$TMP/dir2/instance_4.json"
{ "name": "Diana" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/dir1" "$TMP/dir2" 2> "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
