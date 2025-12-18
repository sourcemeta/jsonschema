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
    "name": { "type": "string" }
  }
}
EOF

mkdir "$TMP/instances"

cat << 'EOF' > "$TMP/instances/instance1"
name: Alice
EOF

cat << 'EOF' > "$TMP/instances/instance2"
name: Bob
EOF

cat << 'EOF' > "$TMP/instances/ignored.yaml"
name: Should be ignored
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --extension '' --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
warning: Matching files with no extension
ok: $(realpath "$TMP")/instances/instance1
  matches $(realpath "$TMP")/schema.json
ok: $(realpath "$TMP")/instances/instance2
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
