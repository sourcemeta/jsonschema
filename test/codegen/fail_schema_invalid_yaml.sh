#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
type: string
---
type: integer
EOF

"$1" codegen "$TMP/schema.yaml" --target typescript 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: Unexpected content after document
  at line 3
  at column 1
  at file path $(realpath "$TMP")/schema.yaml
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" codegen "$TMP/schema.yaml" --target typescript --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Unexpected content after document",
  "line": 3,
  "column": 1,
  "filePath": "$(realpath "$TMP")/schema.yaml"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
