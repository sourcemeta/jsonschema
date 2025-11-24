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
---
foo: "good"
---
foo: "also good"
EOF

"$1" validate "$TMP/schema.yaml" "$TMP/instance.yaml" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "0" || exit 1

# Should be empty
cat << EOF > "$TMP/expected.txt"
EOF

diff -NuarwbB "$TMP/stderr.txt" "$TMP/expected.txt"


