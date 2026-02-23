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
  "$schema": "https://example.com/unknown-metaschema",
  "$id": "https://example.com/resolve",
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
error: Could not resolve the metaschema of the schema
  at identifier https://example.com/unknown-metaschema
  at file path $(realpath "$TMP")/resolve.json

This is likely because you forgot to import such schema using \`--resolve/-r\`
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/resolve.json" --json >"$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Could not resolve the metaschema of the schema",
  "identifier": "https://example.com/unknown-metaschema",
  "filePath": "$(realpath "$TMP")/resolve.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
