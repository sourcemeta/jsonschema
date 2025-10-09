#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/foo"
mkdir -p "$TMP/bar"

cat << 'EOF' > "$TMP/foo/schema.json"
{
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./foo"
}
EOF

cd "$TMP/bar"
"$1" fmt --verbose 2> "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Formatting: $(realpath "$TMP")/foo/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "title": "Hello World",
  "properties": {
    "bar": {},
    "foo": {}
  },
  "additionalProperties": false
}
EOF

diff "$TMP/foo/schema.json" "$TMP/expected.json"
