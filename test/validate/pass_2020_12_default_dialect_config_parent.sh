#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/level1/level2/level3"

cat << 'EOF' > "$TMP/level1/level2/level3/schema.json"
{
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

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

"$1" validate --verbose "$TMP/level1/level2/level3/schema.json" "$TMP/instance.json" 2> "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/level1/level2/level3/schema.json
annotation: "foo"
  at instance location ""
  at evaluate path "/properties"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
