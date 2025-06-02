#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{ "foo": "bar" }
EOF

"$1" lint "$TMP/schema.json" >"$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not determine the base dialect of the schema

Are you sure the input is a valid JSON Schema and its base dialect is known?
If the input does not declare the \$schema keyword, you might want to
explicity declare a default dialect using --default-dialect/-d
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
