#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
id: https://example.com
$schema: http://json-schema.org/draft-04/schema#
type: string
EOF

cat << 'EOF' > "$TMP/test.yaml"
target: https://example.com
tests:
- valid: true
  data: foo
- valid: false
  data: 1
EOF

"$1" test "$TMP/test.yaml" --resolve "$TMP/schema.yaml" --verbose 1> "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Detecting schema resources from file: $(realpath "$TMP")/schema.yaml
Importing schema into the resolution context: https://example.com
$(realpath "$TMP")/test.yaml:
  1/2 PASS <no description>
  2/2 PASS <no description>
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
