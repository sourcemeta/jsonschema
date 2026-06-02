#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "has spaces in it",
  "type": "string"
}
EOF

"$1" metaschema "$TMP/schema.json" --format-assertion 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Metaschema validation failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/schema.json
error: Schema validation failure
  The string value "has spaces in it" was expected to represent a valid URI reference
    at instance location "/\$id" (line 3, column 3)
    at evaluate path "/allOf/0/\$ref/properties/\$id/\$ref/format"
  The string value was expected to validate against the referenced schema
    at instance location "/\$id" (line 3, column 3)
    at evaluate path "/allOf/0/\$ref/properties/\$id/\$ref"
  The object value was expected to validate against the 9 defined properties subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/allOf/0/\$ref/properties"
  The object value was expected to validate against the referenced schema
    at instance location "" (line 1, column 1)
    at evaluate path "/allOf/0/\$ref"
  The object value was expected to validate against the 7 given subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/allOf"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
