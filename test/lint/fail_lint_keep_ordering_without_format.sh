#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "type": "string"
}
EOF

"$1" lint "$TMP/schema.json" --fix --keep-ordering >"$TMP/stderr.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The --keep-ordering option requires --format to be set
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" lint "$TMP/schema.json" --fix --keep-ordering --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The --keep-ordering option requires --format to be set"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
