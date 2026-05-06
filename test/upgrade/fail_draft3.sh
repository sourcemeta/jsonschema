#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-03/schema#",
  "id": "https://example.com",
  "type": "string"
}
EOF

"$1" upgrade "$TMP/schema.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3"

cat << EOF > "$TMP/expected.txt"
error: Upgrading schemas from this dialect is not supported yet
  at line 1
  at column 1
  at file path $(realpath "$TMP")/schema.json
  at location ""
  at uri http://json-schema.org/draft-03/schema#
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" upgrade --json "$TMP/schema.json" > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Upgrading schemas from this dialect is not supported yet",
  "line": 1,
  "column": 1,
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "",
  "uri": "http://json-schema.org/draft-03/schema#"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected_json.txt"
