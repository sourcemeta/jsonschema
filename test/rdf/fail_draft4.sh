#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object",
  "x-jsonld-type": "https://schema.org/Person",
  "properties": {
    "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "Ada" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: This command requires the schema to declare JSON Schema 2019-09 or newer
  at identifier http://json-schema.org/draft-04/schema#
  at file path $(realpath "$TMP")/schema.json

The x-jsonld-* keywords rely on annotation collection, which JSON Schema
only introduced in the 2019-09 dialect. Consider running the \`upgrade\`
command to move your schema to a newer dialect
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" rdf "$TMP/schema.json" "$TMP/instance.json" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
{
  "error": "This command requires the schema to declare JSON Schema 2019-09 or newer",
  "identifier": "http://json-schema.org/draft-04/schema#",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
