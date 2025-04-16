#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance_1.json"
{ "foo": "bar" }
EOF

cat << 'EOF' > "$TMP/instance_2.json"
{ "foo": "baz" }
EOF

cat << 'EOF' > "$TMP/instance_3.json"
{ "foo": "qux" }
EOF

"$1" validate "$TMP/schema.json" \
  "$TMP/instance_1.json" \
  "$TMP/instance_2.json" \
  "$TMP/instance_3.json" \
  --verbose 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance_1.json
  matches $(realpath "$TMP")/schema.json
annotation: "foo"
  at instance location ""
  at evaluate path "/properties"
ok: $(realpath "$TMP")/instance_2.json
  matches $(realpath "$TMP")/schema.json
annotation: "foo"
  at instance location ""
  at evaluate path "/properties"
ok: $(realpath "$TMP")/instance_3.json
  matches $(realpath "$TMP")/schema.json
annotation: "foo"
  at instance location ""
  at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
