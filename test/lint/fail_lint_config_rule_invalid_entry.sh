#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "rules": [ 1 ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: The values in the lint rules array must be strings or objects
  at file path $(realpath "$TMP")/jsonschema.json
  at location "/lint/rules/0"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cd "$TMP"
"$1" lint --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "The values in the lint rules array must be strings or objects",
  "filePath": "$(realpath "$TMP")/jsonschema.json",
  "location": "/lint/rules/0"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
