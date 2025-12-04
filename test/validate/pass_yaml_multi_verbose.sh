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

cat << 'EOF' > "$TMP/instance.yaml"
---
foo: first
---
foo: second
---
foo: third
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.yaml" --verbose 2> "$TMP/output.txt" 1>&2

cat << EOF > "$TMP/expected.txt"
Interpreting input as YAML multi-document: $(realpath "$TMP")/instance.yaml
ok: $(realpath "$TMP")/instance.yaml (entry #1)
  matches $(realpath "$TMP")/schema.json
annotation: "foo"
  at instance location "" (line 2, column 1)
  at evaluate path "/properties"
ok: $(realpath "$TMP")/instance.yaml (entry #2)
  matches $(realpath "$TMP")/schema.json
annotation: "foo"
  at instance location "" (line 4, column 1)
  at evaluate path "/properties"
ok: $(realpath "$TMP")/instance.yaml (entry #3)
  matches $(realpath "$TMP")/schema.json
annotation: "foo"
  at instance location "" (line 6, column 1)
  at evaluate path "/properties"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
