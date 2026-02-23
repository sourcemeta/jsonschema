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
  "$schema": "https://json-schema.org/draft/2019-09/schema",
  "$id": "https://example.com/bad-recref",
  "$recursiveRef": "not-hash"
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
error: Invalid recursive reference
  at identifier https://example.com/bad-recref
  at file path $(realpath "$TMP")/resolve.json
  at location "/\$recursiveRef"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/resolve.json" --json >"$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Invalid recursive reference",
  "identifier": "https://example.com/bad-recref",
  "filePath": "$(realpath "$TMP")/resolve.json",
  "location": "/\$recursiveRef"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
