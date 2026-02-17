#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

# Schema from stdin + instance from stdin is not allowed
"$1" validate - "$TMP/instance.json" - 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: Cannot read both schema and instance from standard input
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
