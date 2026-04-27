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

cat << 'EOF' > "$TMP/instance.jsonl.gz"
this is not valid gzip data
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl.gz" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: Could not decompress gzip stream
  at file path $(realpath "$TMP")/instance.jsonl.gz
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl.gz" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Could not decompress gzip stream",
  "filePath": "$(realpath "$TMP")/instance.jsonl.gz"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
