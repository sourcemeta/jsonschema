#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object"
}
EOF

"$1" validate "$TMP/schema.json" "$TMP/nonexistent.jsonl.gz" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: No such file or directory
  at file path $(realpath "$TMP")/nonexistent.jsonl.gz
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/nonexistent.jsonl.gz" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
{
  "error": "No such file or directory",
  "filePath": "$(realpath "$TMP")/nonexistent.jsonl.gz"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
