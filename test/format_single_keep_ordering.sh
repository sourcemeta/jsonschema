#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

"$1" fmt --keep-ordering "$TMP/schema.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {
    "foo": {},
    "bar": {}
  }
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
