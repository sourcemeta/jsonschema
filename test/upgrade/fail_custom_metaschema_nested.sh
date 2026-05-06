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
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/main",
  "$defs": {
    "foo": {
      "$schema": "https://example.com/custom-metaschema",
      "$id": "https://example.com/foo",
      "type": "string"
    }
  }
}
EOF

"$1" upgrade --resolve "$TMP/metaschema.json" "$TMP/schema.json" \
  2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Cannot upgrade a schema that uses a custom meta-schema
  at line 5
  at column 5
  at file path $(realpath "$TMP")/schema.json
  at location "/\$defs/foo"
  at uri https://example.com/custom-metaschema

Schemas that declare a custom meta-schema cannot be upgraded in place
by this command. Please upgrade the meta-schema and the schema manually.
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" upgrade --resolve "$TMP/metaschema.json" --json "$TMP/schema.json" \
  > "$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Cannot upgrade a schema that uses a custom meta-schema",
  "line": 5,
  "column": 5,
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/\$defs/foo",
  "uri": "https://example.com/custom-metaschema"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected_json.txt"
