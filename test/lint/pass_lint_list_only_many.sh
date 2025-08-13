#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" lint --list --only then_empty --only then_without_if > "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.txt"
then_empty
  Setting the `then` keyword to the empty schema does not add any further constraint

then_without_if
  The `then` keyword is meaningless without the presence of the `if` keyword

Number of rules: 2
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
