#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema"
$schema: http://json-schema.org/draft-06/schema#
type: string
enum: [ foo ]
EOF

"$1" lint "$TMP/schema" --fix 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The --fix option is not supported for YAML input files
  at file path $(realpath "$TMP")/schema
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
