#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

# Schema from stdin + instance from stdin JSON error output
"$1" validate - "$TMP/instance.json" - --json >"$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "error": "Cannot read both schema and instance from standard input"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
