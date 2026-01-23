#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/metaschema.json"
{
  "$id": "https://example.com/custom-metaschema-draft7",
  "$schema": "http://json-schema.org/draft-07/schema#",
  "allOf": [
    { "$ref": "#/$defs/not-a-schema" }
  ],
  "$defs": {
    "not-a-schema": {
      "type": "object"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/custom-metaschema-draft7",
  "type": "string"
}
EOF

"$1" metaschema "$TMP/schema.json" --resolve "$TMP/metaschema.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use
  at identifier https://example.com/custom-metaschema-draft7#/\$defs/not-a-schema
  at file path $(realpath "$TMP")/schema.json
  at location "/\$defs"

Maybe you meant to use \`definitions\` instead of \`\$defs\` in this dialect?
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" metaschema "$TMP/schema.json" --resolve "$TMP/metaschema.json" --json >"$TMP/stdout.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The referenced schema is not considered to be a valid subschema given the dialect and vocabularies in use",
  "identifier": "https://example.com/custom-metaschema-draft7#/\$defs/not-a-schema",
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/\$defs"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
