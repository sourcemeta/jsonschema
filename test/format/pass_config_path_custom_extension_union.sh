#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/foo"
mkdir -p "$TMP/bar"

# This file should be formatted (matches .schema.json from config)
cat << 'EOF' > "$TMP/foo/example.schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "description": "Test schema",
  "additionalProperties": false,
  "title": "Schema JSON",
  "properties": {"foo": {}, "bar": {}}
}
EOF

# This file should be formatted (matches .my.json from --extension)
cat << 'EOF' > "$TMP/foo/example.my.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "description": "Test schema",
  "additionalProperties": false,
  "title": "My JSON",
  "properties": {"foo": {}, "bar": {}}
}
EOF

# This file should NOT be formatted (only .json, not .schema.json or .my.json)
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
"$1" fmt --verbose --extension .my.json >"$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using extension: .my.json
Using extension: .schema.json
Formatting: $(realpath "$TMP")/foo/example.my.json
Formatting: $(realpath "$TMP")/foo/example.schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected_schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Schema JSON",
  "description": "Test schema",
  "properties": {
    "foo": {},
    "bar": {}
  },
  "additionalProperties": false
}
EOF

diff "$TMP/foo/example.schema.json" "$TMP/expected_schema.json"

cat << 'EOF' > "$TMP/expected_my.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "My JSON",
  "description": "Test schema",
  "properties": {
    "foo": {},
    "bar": {}
  },
  "additionalProperties": false
}
EOF

diff "$TMP/foo/example.my.json" "$TMP/expected_my.json"

cat << 'EOF' > "$TMP/expected_other.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "additionalProperties": false,
  "title": "Other",
  "properties": {"foo": {}, "bar": {}}
}
EOF

diff "$TMP/foo/other.json" "$TMP/expected_other.json"
