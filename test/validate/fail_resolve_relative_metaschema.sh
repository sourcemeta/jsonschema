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

cat << 'EOF' > "$TMP/resolve.json"
{
  "$schema": "relative/path",
  "$id": "https://example.com/relative-test",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/resolve.json" 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Relative meta-schema URIs are not valid according to the JSON Schema specification
  at identifier relative/path
  at file path $(realpath "$TMP")/resolve.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/resolve.json" --json >"$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Relative meta-schema URIs are not valid according to the JSON Schema specification",
  "identifier": "relative/path",
  "filePath": "$(realpath "$TMP")/resolve.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
