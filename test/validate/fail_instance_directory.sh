#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

mkdir "$TMP/instance"

"$1" validate "$TMP/schema.json" "$TMP/instance" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The input was supposed to be a file but it is a directory
  $(realpath "$TMP")/instance
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
