#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" test - 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
[ 1, 2, 3 ]
EOF
# Other input error
test "$EXIT_CODE" = "6"

cat << 'EOF' > "$TMP/expected.txt"
/dev/stdin:
error: The test document must be an object
  at line 1
  at column 1
  at file path /dev/stdin
  at location ""

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
