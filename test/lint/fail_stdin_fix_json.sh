#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

# lint --fix does not support stdin (JSON error output)
cat << 'EOF' | "$1" lint --fix - --json >"$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "error": "The --fix option does not support standard input"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
