#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/metaschema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/custom-metaschema",
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/custom-metaschema",
  "$id": "https://example.com/test",
  "type": "string"
}
EOF

"$1" upgrade --resolve "$TMP/metaschema.json" "$TMP/schema.json" \
  2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Cannot upgrade a schema that uses a custom meta-schema
  at line 1
  at column 1
  at file path $(realpath "$TMP")/schema.json
  at location ""
  at uri https://example.com/custom-metaschema

Schemas that declare a custom meta-schema cannot be upgraded in place
by this command. Please upgrade the meta-schema and the schema manually.
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" upgrade --resolve "$TMP/metaschema.json" --json "$TMP/schema.json" \
  > "$TMP/stdout.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Cannot upgrade a schema that uses a custom meta-schema",
  "line": 1,
  "column": 1,
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "",
  "uri": "https://example.com/custom-metaschema"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected_json.txt"
