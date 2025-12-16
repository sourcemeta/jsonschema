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
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance_1.json"
{ "foo": "bar" }
EOF

cat << 'EOF' > "$TMP/instance_2.json"
{ "foo": "baz" }
EOF

cat << 'EOF' > "$TMP/instance_3.json"
{ "foo": "qux" }
EOF

"$1" validate "$TMP/schema.json" \
  "$TMP/instance_1.json" \
  "$TMP/instance_2.json" \
  "$TMP/instance_3.json" \
  2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
