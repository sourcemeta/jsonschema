#!/bin/sh
set -o errexit
set -o nounset
TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT
cat << 'SCHEMA_EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
SCHEMA_EOF
"$1" inspect "$TMP/schema.json" --keywords --json > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "5"
cat << 'EXPECTED_EOF' > "$TMP/expected.txt"
{
  "error": "The --keywords option cannot be used with --json"
}
EXPECTED_EOF
diff "$TMP/output.txt" "$TMP/expected.txt"
