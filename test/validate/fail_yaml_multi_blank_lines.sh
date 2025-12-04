#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object"
}
EOF

# YAML multi-document with blank lines around separators
cat << 'EOF' > "$TMP/instance.yaml"
---
foo: first

---

- bar: second
---
baz: third
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.yaml" --verbose 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
Interpreting input as YAML multi-document: $(realpath "$TMP")/instance.yaml
ok: $(realpath "$TMP")/instance.yaml (entry #1)
  matches $(realpath "$TMP")/schema.json
fail: $(realpath "$TMP")/instance.yaml (entry #2)

[
  {
    "bar": "second"
  }
]

error: Schema validation failure
  The value was expected to be of type object but it was of type array
    at instance location "" (line 6, column 1)
    at evaluate path "/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
