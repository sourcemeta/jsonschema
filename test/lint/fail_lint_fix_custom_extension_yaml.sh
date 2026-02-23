#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.custom"
$schema: http://json-schema.org/draft-06/schema#
type: string
enum: [ foo ]
EOF

"$1" lint "$TMP/schema.custom" --fix 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The --fix option is not supported for YAML input files
  at file path $(realpath "$TMP")/schema.custom
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
