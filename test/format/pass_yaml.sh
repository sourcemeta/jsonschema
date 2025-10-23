#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
additionalProperties: false
title: Hello World
EOF

"$1" fmt "$TMP/schema.yaml" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/error.txt"
This command does not support YAML input files yet
  $(realpath "$TMP/schema.yaml")
EOF

diff "$TMP/stderr.txt" "$TMP/error.txt"

cat << EOF > "$TMP/expected.yaml"
additionalProperties: false
title: Hello World
EOF

diff "$TMP/schema.yaml" "$TMP/expected.yaml"
