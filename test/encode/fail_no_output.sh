#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{ "version": 2.0 }
EOF

"$1" encode "$TMP/document.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: This command expects a path to a JSON document and an output path. For example:

  jsonschema encode path/to/document.json path/to/output.binpack
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
