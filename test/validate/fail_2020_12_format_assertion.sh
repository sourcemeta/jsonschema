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
  "$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true,
    "https://json-schema.org/draft/2020-12/vocab/format-assertion": true
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/custom-metaschema",
  "type": "object",
  "properties": {
    "email": {
      "type": "string",
      "format": "email"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "email": "not-an-email" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/metaschema.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Cannot compile unsupported vocabulary
  at file path $(realpath "$TMP")/schema.json
  at uri https://json-schema.org/draft/2020-12/vocab/format-assertion
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/metaschema.json" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Cannot compile unsupported vocabulary",
  "filePath": "$(realpath "$TMP")/schema.json",
  "uri": "https://json-schema.org/draft/2020-12/vocab/format-assertion"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
