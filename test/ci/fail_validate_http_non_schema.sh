#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "allOf": [ { "$ref": "https://schemas.sourcemeta.com/self/api/schemas/stats/jsonschema/2020-12/schema" } ]
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "type": "string" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --http 2> "$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The JSON document is not a valid JSON Schema
  at identifier https://schemas.sourcemeta.com/self/api/schemas/stats/jsonschema/2020-12/schema
  at location "/allOf/0/\$ref"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
