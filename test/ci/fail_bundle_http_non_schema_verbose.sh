#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "allOf": [ { "$ref": "https://jsonplaceholder.typicode.com/todos/1" } ]
}
EOF

"$1" bundle "$TMP/schema.json" --http --verbose 2> "$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Resolving over HTTP: https://jsonplaceholder.typicode.com/todos/1
error: The JSON document is not a valid JSON Schema
  https://jsonplaceholder.typicode.com/todos/1
    at schema location "/allOf/0/\$ref"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
