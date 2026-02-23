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
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "not a valid uri ::",
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
error: The identifier is not a valid URI
  at value not a valid uri ::
  at keyword \$id
  at file path $(realpath "$TMP")/resolve.json

Are you sure the input is a valid JSON Schema and it is valid according to its meta-schema?
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/resolve.json" --json >"$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The identifier is not a valid URI",
  "value": "not a valid uri ::",
  "keyword": "\$id",
  "filePath": "$(realpath "$TMP")/resolve.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
