#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" validate 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: This command expects a path to a schema and a path to an
instance to validate against the schema

For example: jsonschema validate path/to/schema.json path/to/instance.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate --json > "$TMP/stdout.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "This command expects a path to a schema and a path to an\ninstance to validate against the schema"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
