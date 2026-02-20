#!/bin/sh
set -e
TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

echo '"foo"' | "$1" validate "$TMP/schema.json" - - --json > "$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "error": "Cannot read from standard input more than once"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
