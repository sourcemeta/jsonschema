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

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cd "$TMP"

"$1" fmt schema.json --verbose 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "title": "Hello World",
  "properties": {
    "foo": {},
    "bar": {}
  },
  "additionalProperties": false
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"

cat << EOF > "$TMP/expected_log.txt"
Formatting: $(realpath "$TMP")/schema.json
Using configuration file: $(realpath "$TMP")/jsonschema.json
EOF

diff "$TMP/output.txt" "$TMP/expected_log.txt"
