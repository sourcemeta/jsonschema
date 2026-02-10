#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cd "$TMP"

"$1" install > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: Could not find a jsonschema.json configuration file
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --json > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "Could not find a jsonschema.json configuration file"
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
