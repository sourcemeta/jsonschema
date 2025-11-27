#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
$schema: https://json-schema.org/draft/2019-09/schema
type: object
properties:
  foo:
    type: string
EOF

cat << 'EOF' > "$TMP/instance.yaml"
foo: "test"
EOF

"$1" validate --verbose "$TMP/schema.yaml" "$TMP/instance.yaml" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "0" || exit 1

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance.yaml
  matches $(realpath "$TMP")/schema.yaml
annotation: "foo"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
