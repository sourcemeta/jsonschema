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
    "name": {
      "type": "string"
    }
  }
}
EOF

mkdir "$TMP/instances"

cat << 'EOF' > "$TMP/instances/instance_1.data.json"
{ "name": "Alice" }
EOF

cat << 'EOF' > "$TMP/instances/instance_2.data.json"
{ "name": "Bob" }
EOF

cat << 'EOF' > "$TMP/instances/other.json"
{ "name": 123 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --extension .data.json --verbose 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
Using extension: .data.json
ok: $(realpath "$TMP")/instances/instance_1.data.json
  matches $(realpath "$TMP")/schema.json
annotation: "name"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
ok: $(realpath "$TMP")/instances/instance_2.data.json
  matches $(realpath "$TMP")/schema.json
annotation: "name"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
