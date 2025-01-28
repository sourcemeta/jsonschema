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
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schemas/schema_2.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF

"$1" metaschema "$TMP/schemas" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/schemas/schema_2.json
error: Schema validation failure
  The integer value 1 was expected to equal one of the given declared values
    at instance location "/type"
    at evaluate path "/properties/type/anyOf/0/\$ref/enum"
  The value was expected to consist of an array of at least 1 item
    at instance location "/type"
    at evaluate path "/properties/type/anyOf/1/type"
  The integer value was expected to validate against at least one of the 2 given subschemas
    at instance location "/type"
    at evaluate path "/properties/type/anyOf"
  The object value was expected to validate against the 33 defined properties subschemas
    at instance location ""
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

