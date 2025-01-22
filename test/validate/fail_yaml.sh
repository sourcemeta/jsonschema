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
foo: 1
EOF

"$1" validate "$TMP/schema.yaml" "$TMP/instance.yaml" 2> "$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.yaml
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "/foo"
    at evaluate path "/properties/foo/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
