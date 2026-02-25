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

echo '123' | "$1" validate "$TMP/schema.json" - >"$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(pwd)
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "" (line 1, column 1)
    at evaluate path "/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
