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
  --entrypoint 'https://example.com/foo bar' > "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The given entry point is not a valid URI or JSON Pointer
  at identifier https://example.com/foo bar
  at file path $(realpath "$TMP")/schema.json

Use the \`inspect\` command to find valid schema locations
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --entrypoint 'https://example.com/foo bar' --json > "$TMP/stdout.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The given entry point is not a valid URI or JSON Pointer",
  "identifier": "https://example.com/foo bar",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
