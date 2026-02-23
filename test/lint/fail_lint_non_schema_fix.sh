#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{ "foo": "bar" }
EOF

"$1" lint "$TMP/schema.json" --fix >"$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not determine the base dialect of the schema
  at file path $(realpath "$TMP/schema.json")

Are you sure the input is a valid JSON Schema and its base dialect is known?
If the input does not declare the \`\$schema\` keyword, you might want to
explicitly declare a default dialect using \`--default-dialect/-d\`
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
