#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type": "string",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema"
}
EOF

"$1" fmt "$TMP/schema.json" --check >"$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Format check failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/schema.json

Run the \`fmt\` command without \`--check/-c\` to fix the formatting
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "type": "string",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
