#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" bundle 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: This command expects a path to a schema

For example: jsonschema bundle path/to/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
