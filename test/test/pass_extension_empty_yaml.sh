#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"
mkdir "$TMP/tests"

cat << 'EOF' > "$TMP/schemas/schema"
id: https://example.com
$schema: http://json-schema.org/draft-04/schema#
type: string
EOF

cat << 'EOF' > "$TMP/tests/test1"
target: https://example.com
tests:
- description: First test
  valid: true
  data: foo
EOF

cat << 'EOF' > "$TMP/tests/test2"
target: https://example.com
tests:
- description: Second test
  valid: false
  data: 1
EOF

cat << 'EOF' > "$TMP/tests/ignored.yaml"
target: https://example.com
tests:
- description: Ignored test
  valid: true
  data: should not run
EOF

"$1" test "$TMP/tests" --resolve "$TMP/schemas" --extension '' --verbose 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
warning: Matching files with no extension
Detecting schema resources from file: $(realpath "$TMP")/schemas/schema
Importing schema into the resolution context: file://$(realpath "$TMP")/schemas/schema
Importing schema into the resolution context: https://example.com
$(realpath "$TMP")/tests/test1:
  1/1 PASS First test
$(realpath "$TMP")/tests/test2:
  1/1 PASS Second test
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
