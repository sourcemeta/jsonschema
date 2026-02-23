#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

# This error is not handled specifically by the CLI because ideally it would
# never happen. It is only thrown due to temporary limitations in the codegen
# module, and should be resolved as the module matures.

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "not": { "type": "string" }
}
EOF

"$1" codegen "$TMP/schema.json" --target typescript \
  2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Internal error
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
unexpected error: Unsupported schema
Please report it at https://github.com/sourcemeta/jsonschema
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" codegen "$TMP/schema.json" --target typescript --json \
  >"$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Internal error
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "Unsupported schema"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
