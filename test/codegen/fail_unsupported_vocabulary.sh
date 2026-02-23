#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-03/schema#",
  "type": "string"
}
EOF

"$1" codegen "$TMP/schema.json" --target typescript \
  2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Unsupported required vocabulary
  at file path $(realpath "$TMP")/schema.json
  at uri http://json-schema.org/draft-03/schema#
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" codegen "$TMP/schema.json" --target typescript --json \
  >"$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Unsupported required vocabulary",
  "filePath": "$(realpath "$TMP")/schema.json",
  "uri": "http://json-schema.org/draft-03/schema#"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
