#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
  {
             "$schema":
   "http://json-schema.org/draft-06/schema#",    "title": "Test", "description": "Test schema", "examples": [ "foo" ], "type"    : "string"
}
EOF

"$1" lint "$TMP/schema.json" --fix

# If no linting rule applies, we should leave the schema intact
cat << 'EOF' > "$TMP/expected.json"
  {
             "$schema":
   "http://json-schema.org/draft-06/schema#",    "title": "Test", "description": "Test schema", "examples": [ "foo" ], "type"    : "string"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
