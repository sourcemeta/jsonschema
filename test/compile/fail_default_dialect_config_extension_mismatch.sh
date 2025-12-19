#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{ "foo": 1 }
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "extension": [ ".schema.json" ],
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

"$1" compile "$TMP/document.json" --verbose 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Ignoring configuration file given extensions mismatch: $(realpath "$TMP")/jsonschema.json
error: Could not determine the base dialect of the schema
  at file path $(realpath "$TMP")/document.json

Are you sure the input is a valid JSON Schema and its base dialect is known?
If the input does not declare the \`\$schema\` keyword, you might want to
explicitly declare a default dialect using \`--default-dialect/-d\`
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" compile "$TMP/document.json" --json >"$TMP/stdout.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Could not determine the base dialect of the schema",
  "filePath": "$(realpath "$TMP")/document.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
