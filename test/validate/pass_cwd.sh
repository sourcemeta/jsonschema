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

cat << 'EOF' > "$TMP/instance_1.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/instance_2.json"
{ "name": "Bob" }
EOF

BIN="$(realpath "$1")"
cd "$TMP"
"$BIN" validate schema.json --verbose 2> "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance_1.json
  matches $(realpath "$TMP")/schema.json
ok: $(realpath "$TMP")/instance_2.json
  matches $(realpath "$TMP")/schema.json
ok: $(realpath "$TMP")/schema.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
