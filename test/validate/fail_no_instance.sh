#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

"$1" validate "$TMP/schema.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: In addition to the schema, you must also pass an argument
that represents the instance to validate against

For example: jsonschema validate path/to/schema.json path/to/instance.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
