#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" lint --fix --format - > "$TMP/output.json" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string",
  "const": "foo",
  "title": "I should not be moved up"
}
EOF
test "$EXIT_CODE" = "0" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "I should not be moved up",
  "description": "Test schema",
  "examples": [ "foo" ],
  "const": "foo"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
