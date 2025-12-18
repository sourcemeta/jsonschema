#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema1"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "examples": [ "foo" ]
}
EOF

cat << 'EOF' > "$TMP/schemas/schema2"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Test 2",
  "description": "Test schema 2",
  "type": "number",
  "examples": [ 1 ]
}
EOF

cat << 'EOF' > "$TMP/schemas/ignored.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Ignored",
  "description": "This file should be ignored",
  "type": "boolean",
  "examples": [ true ]
}
EOF

"$1" lint "$TMP/schemas" --extension '' --verbose >"$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
warning: Matching files with no extension
Linting: $(realpath "$TMP")/schemas/schema1
Linting: $(realpath "$TMP")/schemas/schema2
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
