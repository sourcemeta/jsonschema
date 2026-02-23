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

"$1" fmt "$TMP/schema.yaml" >"$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3" || exit 1

cat << EOF > "$TMP/expected.txt"
error: This command does not support YAML input files yet
  at file path $(realpath "$TMP/schema.yaml")
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected.yaml"
additionalProperties: false
title: Hello World
EOF

diff "$TMP/schema.yaml" "$TMP/expected.yaml"
