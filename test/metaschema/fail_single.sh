#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF

"$1" metaschema "$TMP/schema.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/schema.json
error: Schema validation failure
  The integer value was expected to validate against at least one of the 2 given subschemas
    at instance location "/type" (line 3, column 3)
    at evaluate path "/properties/type/anyOf"
  The object value was expected to validate against the 33 defined properties subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
