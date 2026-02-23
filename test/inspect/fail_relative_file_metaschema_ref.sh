#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/schemas/folder"

cat << 'EOF' > "$TMP/schemas/folder/test.json"
{ "$schema": "../meta.json" }
EOF

cat << 'EOF' > "$TMP/schemas/meta.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$vocabulary": { "https://json-schema.org/draft/2020-12/vocab/core": true },
  "$dynamicAnchor": "meta",
  "$ref": "https://json-schema.org/draft/2020-12/meta/core"
}
EOF

"$1" inspect "$TMP/schemas/folder/test.json" \
  --resolve "$TMP/schemas/meta.json" > "$TMP/result.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Relative meta-schema URIs are not valid according to the JSON Schema specification
  at identifier ../meta.json
  at file path $(realpath "$TMP")/schemas/folder/test.json
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
