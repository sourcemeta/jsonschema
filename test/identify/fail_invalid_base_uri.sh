#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://www.example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema"
}
EOF

"$1" identify "$TMP/schema.json" \
  --relative-from "111https://////" >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Invalid base URI at column 9
  111https://////
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
