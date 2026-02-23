#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

# Test with hyphen (invalid)
"$1" compile "$TMP/schema.json" --include my-schema 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: The include identifier is not a valid C/C++ identifier
  at identifier my-schema
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" compile "$TMP/schema.json" --include my-schema --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "The include identifier is not a valid C/C++ identifier",
  "identifier": "my-schema"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
