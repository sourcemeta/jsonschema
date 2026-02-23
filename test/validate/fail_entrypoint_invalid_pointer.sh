#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --entrypoint '/nonexistent' > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The given entry point URI does not exist in the schema
  at identifier file://$(realpath "$TMP")/schema.json#/nonexistent
  at file path $(realpath "$TMP")/schema.json

Use the \`inspect\` command to find valid schema locations
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --entrypoint '/nonexistent' --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The given entry point URI does not exist in the schema",
  "identifier": "file://$(realpath "$TMP")/schema.json#/nonexistent",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
