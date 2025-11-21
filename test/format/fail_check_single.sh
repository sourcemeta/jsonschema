#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

"$1" fmt "$TMP/schema.json" --check >"$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/schema.json

Run the \`fmt\` command without \`--check/-c\` to fix the formatting
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
