#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" validate 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: This command expects a path to a schema and a path to an
instance to validate against the schema

For example: jsonschema validate path/to/schema.json path/to/instance.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
