#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "x-jsonld-type": "https://schema.org/Person"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

cat << 'EOF' > "$TMP/context.json"
{ "name" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" \
  --compact "$TMP/context.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 1
  at column 10
  at file path $(realpath "$TMP")/context.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" rdf "$TMP/schema.json" "$TMP/instance.json" \
  --compact "$TMP/context.json" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 1,
  "column": 10,
  "filePath": "$(realpath "$TMP")/context.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
