#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

echo '{ "type" string }' | "$1" validate - "$TMP/instance.json" 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "6" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 1
  at column 10
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
echo '{ "type" string }' | "$1" validate - "$TMP/instance.json" --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "6" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 1,
  "column": 10
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
