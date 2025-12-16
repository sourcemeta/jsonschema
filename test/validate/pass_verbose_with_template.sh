#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" compile "$TMP/schema.json" > "$TMP/template.json"
"$1" validate "$TMP/schema.json" "$TMP/instance.json" --verbose \
  --template "$TMP/template.json" 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
Parsing pre-compiled schema template: $(realpath "$TMP")/template.json
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
annotation: "Test schema"
  at instance location "" (line 1, column 1)
  at evaluate path "/description"
annotation: "foo"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
annotation: "Test"
  at instance location "" (line 1, column 1)
  at evaluate path "/title"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
