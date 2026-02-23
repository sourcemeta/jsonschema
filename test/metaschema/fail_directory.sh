#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema_1.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schemas/schema_2.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": 1
}
EOF

"$1" metaschema "$TMP/schemas" 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Metaschema validation failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/schemas/schema_2.json
error: Schema validation failure
  The integer value 1 was expected to equal one of the following values: "array", "boolean", "integer", "null", "number", "object", and "string"
    at instance location "/type" (line 5, column 3)
    at evaluate path "/properties/type/anyOf/0/\$ref/enum"
  The integer value was expected to validate against the referenced schema
    at instance location "/type" (line 5, column 3)
    at evaluate path "/properties/type/anyOf/0/\$ref"
  The value was expected to be of type array but it was of type integer
    at instance location "/type" (line 5, column 3)
    at evaluate path "/properties/type/anyOf/1/type"
  The integer value was expected to validate against at least one of the 2 given subschemas
    at instance location "/type" (line 5, column 3)
    at evaluate path "/properties/type/anyOf"
  The object value was expected to validate against the 33 defined properties subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
