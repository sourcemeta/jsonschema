#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
$schema: http://json-schema.org/draft-04/schema#
type: 1
EOF

"$1" metaschema "$TMP/schema.yaml" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/schema.yaml
error: Schema validation failure
  The integer value 1 was expected to equal one of the following values: "array", "boolean", "integer", "null", "number", "object", and "string"
    at instance location "/type" (line 2, column 1)
    at evaluate path "/properties/type/anyOf/0/\$ref/enum"
  The integer value was expected to validate against the referenced schema
    at instance location "/type" (line 2, column 1)
    at evaluate path "/properties/type/anyOf/0/\$ref"
  The value was expected to be of type array but it was of type integer
    at instance location "/type" (line 2, column 1)
    at evaluate path "/properties/type/anyOf/1/type"
  The integer value was expected to validate against at least one of the 2 given subschemas
    at instance location "/type" (line 2, column 1)
    at evaluate path "/properties/type/anyOf"
  The object value was expected to validate against the 33 defined properties subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
