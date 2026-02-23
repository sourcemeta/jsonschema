#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

"$1" lint "$TMP/schema.json" \
  --only enum_with_type --exclude const_with_type --verbose \
  >"$TMP/stderr.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Cannot use --only and --exclude at the same time
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
