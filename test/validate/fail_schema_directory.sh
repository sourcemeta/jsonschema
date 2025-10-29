#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schema-directory"

cat << 'EOF' > "$TMP/instance.json"
{ "foo": 1 }
EOF

"$1" validate "$TMP/schema-directory" "$TMP/instance.json" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The input was supposed to be a file but it is a directory
  at file path $(realpath "$TMP")/schema-directory
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "$TMP/schema-directory" "$TMP/instance.json" --json >"$TMP/stdout.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The input was supposed to be a file but it is a directory",
  "filePath": "$(realpath "$TMP")/schema-directory"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
