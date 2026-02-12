#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cd "$TMP"

"$1" ci > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not find a jsonschema.json configuration file
  at file path $(realpath "$TMP")

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/install.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" ci --json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Could not find a jsonschema.json configuration file",
  "filePath": "$(realpath "$TMP")"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
