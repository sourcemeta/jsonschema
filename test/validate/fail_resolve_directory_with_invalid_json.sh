#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/01-valid.json"
{ "foo": 1 }
EOF

cat << 'EOF' > "$TMP/schemas/02-invalid.json"
{ xxx }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/schemas" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 1
  at column 3
  at file path $(realpath "$TMP")/schemas/02-invalid.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/schemas" --json >"$TMP/stdout.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 1,
  "column": 3,
  "filePath": "$(realpath "$TMP")/schemas/02-invalid.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
