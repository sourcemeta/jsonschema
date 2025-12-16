#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/schemas/folder"

cat << 'EOF' > "$TMP/schemas/folder/test.json"
{ "$schema": "../meta.json" }
"title": "Test",
"description": "Test schema",
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

"$1" metaschema "$TMP/schemas/folder/test.json" \
  --resolve "$TMP/schemas/meta.json" > "$TMP/result.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Relative meta-schema URIs are not valid according to the JSON Schema specification
  at identifier ../meta.json
  at file path $(realpath "$TMP")/schemas/folder/test.json
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"

# JSON error
"$1" metaschema "$TMP/schemas/folder/test.json" \
  --resolve "$TMP/schemas/meta.json" --json > "$TMP/stdout.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Relative meta-schema URIs are not valid according to the JSON Schema specification",
  "identifier": "../meta.json",
  "filePath": "$(realpath "$TMP")/schemas/folder/test.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
