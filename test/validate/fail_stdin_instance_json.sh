#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

# Should fail validation via stdin (exit code 2)
echo '123' | "$1" validate "$TMP/schema.json" - --json > "$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

# Validate that JSON output has the correct structure
grep -q '"valid": false' "$TMP/output.json" || exit 1
grep -q '"errors"' "$TMP/output.json" || exit 1
grep -q '"keywordLocation"' "$TMP/output.json" || exit 1
