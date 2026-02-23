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

cat << 'EOF' > "$TMP/meta.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/my-meta",
  "$vocabulary": {
    "https://example.com/fake-vocab": true
  }
}
EOF

cat << 'EOF' > "$TMP/resolve.json"
{
  "$schema": "https://example.com/my-meta",
  "$id": "https://example.com/my-schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/meta.json" --resolve "$TMP/resolve.json" 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The core vocabulary must always be present
  at file path $(realpath "$TMP")/resolve.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/meta.json" --resolve "$TMP/resolve.json" --json >"$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The core vocabulary must always be present",
  "filePath": "$(realpath "$TMP")/resolve.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
