#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com/schema",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
EOF

"$1" compile "$TMP/schema.json" \
  --entrypoint 'https://example.com/nonexistent' > "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The given entry point URI does not exist in the schema
  at identifier https://example.com/nonexistent
  at file path $(realpath "$TMP")/schema.json

Use the \`inspect\` command to find valid schema locations
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" compile "$TMP/schema.json" \
  --entrypoint 'https://example.com/nonexistent' --json > "$TMP/stdout.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The given entry point URI does not exist in the schema",
  "identifier": "https://example.com/nonexistent",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
