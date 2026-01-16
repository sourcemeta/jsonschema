#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" codegen "$TMP/schema.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: You must pass a target using the `--target/-t` option
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" codegen "$TMP/schema.json" --json \
  >"$TMP/stdout.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "You must pass a target using the `--target/-t` option"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
