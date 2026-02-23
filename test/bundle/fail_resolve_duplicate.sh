#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com",
  "$ref": "nested"
}
EOF

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/remote.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com/nested",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schemas/duplicated.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com/nested",
  "type": "number"
}
EOF

"$1" bundle "$TMP/schema.json" \
  --resolve "$TMP/schemas" 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Cannot register the same identifier twice
  at identifier https://example.com/nested
  at file path $(realpath "$TMP")/schemas/remote.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" bundle "$TMP/schema.json" \
  --resolve "$TMP/schemas" --json >"$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Cannot register the same identifier twice",
  "identifier": "https://example.com/nested",
  "filePath": "$(realpath "$TMP")/schemas/remote.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
