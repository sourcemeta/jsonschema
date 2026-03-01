#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" validate - "$TMP/instance.json" - 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: Cannot read from standard input more than once
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate - "$TMP/instance.json" - --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "Cannot read from standard input more than once"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
