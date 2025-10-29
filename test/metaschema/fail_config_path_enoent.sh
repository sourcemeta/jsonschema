#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/bar"

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./nonexistent"
}
EOF

cd "$TMP/bar"
"$1" metaschema --verbose > "$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
error: No such file or directory
  at file path $(realpath "$TMP")/nonexistent
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
cd "$TMP/bar"
"$1" metaschema --json > "$TMP/stdout.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "No such file or directory",
  "filePath": "$(realpath "$TMP")/nonexistent"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
