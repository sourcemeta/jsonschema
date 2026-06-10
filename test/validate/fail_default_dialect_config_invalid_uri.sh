#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{ "type": "string" }
EOF

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{ "defaultDialect": ":::not a URI" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
error: The default dialect is not a valid URI reference
  at value :::not a URI
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "The default dialect is not a valid URI reference",
  "value": ":::not a URI"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
