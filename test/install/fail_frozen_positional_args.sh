#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/project"

cat << 'EOF' > "$TMP/project/jsonschema.json"
{
  "dependencies": {}
}
EOF

cd "$TMP/project"
"$1" install --frozen https://example.com/schema ./vendor/schema.json > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: The --frozen option does not accept positional arguments

For example: jsonschema install --frozen
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --frozen --json https://example.com/schema ./vendor/schema.json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "error": "The --frozen option does not accept positional arguments"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
