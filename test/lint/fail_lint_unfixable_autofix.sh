#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cp "$TMP/schema.json" "$TMP/copy.json"

cd "$TMP"
"$1" lint "$TMP/schema.json" --fix >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
schema.json:1:1:
  Set a concise non-empty title at the top level of the schema to explain what the definition is about (top_level_title)
    at location ""
schema.json:1:1:
  Set a non-empty description at the top level of the schema to explain what the definition is about in detail (top_level_description)
    at location ""
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# Verify that the schema was not modified
diff "$TMP/schema.json" "$TMP/copy.json"
