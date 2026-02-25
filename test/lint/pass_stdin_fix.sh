#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" lint --fix - > "$TMP/output.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string",
  "const": "foo",
  "title": "I should not be moved up"
}
EOF

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "description": "Test schema",
  "examples": [ "foo" ],
  "const": "foo",
  "title": "I should not be moved up"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
