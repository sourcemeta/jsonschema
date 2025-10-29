#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{ "version": 2.0 }
EOF

"$1" decode 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: This command expects a path to a binary file and an output path

For example: jsonschema decode path/to/output.binpack path/to/document.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" decode --json > "$TMP/stdout.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "This command expects a path to a binary file and an output path"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
