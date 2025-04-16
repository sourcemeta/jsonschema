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

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --verbose 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
annotation: "foo"
  at instance location ""
  at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
