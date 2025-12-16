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
    "name": {
      "type": "string"
    },
    "age": {
      "type": "integer"
    }
  }
}
EOF

mkdir "$TMP/instances"

cat << 'EOF' > "$TMP/instances/instance_1.json"
{ "name": "Alice", "age": 30 }
EOF

cat << 'EOF' > "$TMP/instances/instance_2.json"
{ "name": "Bob", "age": 25 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --verbose 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instances/instance_1.json
  matches $(realpath "$TMP")/schema.json
annotation: "Test schema"
  at instance location "" (line 1, column 1)
  at evaluate path "/description"
annotation: "age"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
annotation: "name"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
annotation: "Test"
  at instance location "" (line 1, column 1)
  at evaluate path "/title"
ok: $(realpath "$TMP")/instances/instance_2.json
  matches $(realpath "$TMP")/schema.json
annotation: "Test schema"
  at instance location "" (line 1, column 1)
  at evaluate path "/description"
annotation: "age"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
annotation: "name"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
annotation: "Test"
  at instance location "" (line 1, column 1)
  at evaluate path "/title"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
