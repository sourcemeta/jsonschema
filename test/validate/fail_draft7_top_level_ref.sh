#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

# We expect `/type` to be ignored here
cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$ref": "#/definitions/foo",
  "type": "object",
  "definitions": {
    "foo": {}
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: A schema with a top-level \`\$ref\` in JSON Schema Draft 7 and older dialects ignores every sibling keywords (like identifiers and meta-schema declarations) and therefore many operations, like bundling, are not possible without undefined behavior
  at identifier file://$(realpath "$TMP")/schema.json
  at file path $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --json > "$TMP/stdout.json" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "error": "A schema with a top-level \`\$ref\` in JSON Schema Draft 7 and older dialects ignores every sibling keywords (like identifiers and meta-schema declarations) and therefore many operations, like bundling, are not possible without undefined behavior",
  "identifier": "file://$(realpath "$TMP")/schema.json",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.json" "$TMP/expected.json"
