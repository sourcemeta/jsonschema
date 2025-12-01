#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/foo"
mkdir -p "$TMP/bar"

# This file should be formatted (matches .schema.json extension)
cat << 'EOF' > "$TMP/foo/example.schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

# This file should NOT be formatted (only .json, not .schema.json)
cat << 'EOF' > "$TMP/foo/other.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "additionalProperties": false,
  "title": "Other",
  "properties": {"foo": {}, "bar": {}}
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./foo",
  "extension": ".schema.json"
}
EOF

cd "$TMP/bar"
"$1" fmt --verbose >"$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Using extension: .schema.json
Formatting: $(realpath "$TMP")/foo/example.schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Hello World",
  "properties": {
    "foo": {},
    "bar": {}
  },
  "additionalProperties": false
}
EOF

diff "$TMP/foo/example.schema.json" "$TMP/expected.json"

cat << 'EOF' > "$TMP/expected_other.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "additionalProperties": false,
  "title": "Other",
  "properties": {"foo": {}, "bar": {}}
}
EOF

diff "$TMP/foo/other.json" "$TMP/expected_other.json"
