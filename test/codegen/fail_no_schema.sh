#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" codegen 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: This command expects a path to a schema

For example: jsonschema codegen path/to/schema.json --name MyType --target typescript
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" codegen --json > "$TMP/stdout.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "This command expects a path to a schema"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
