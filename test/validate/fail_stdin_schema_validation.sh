#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/instance.json"
123
EOF

cat << 'EOF' | "$1" validate - "$TMP/instance.json" >"$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test-schema",
  "type": "string"
}
EOF
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.json
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "" (line 1, column 1)
    at evaluate path "/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
cat << 'EOF' | "$1" validate - "$TMP/instance.json" --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test-schema",
  "type": "string"
}
EOF
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/type",
      "absoluteKeywordLocation": "https://example.com/test-schema#/type",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 3 ],
      "error": "The value was expected to be of type string but it was of type integer"
    }
  ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"

# Schema from stdin without $id
cat << 'EOF' | "$1" validate - "$TMP/instance.json" >"$TMP/output_no_id.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF
test "$EXIT_CODE" = "2" || exit 1

CWD="$(pwd -P)"

cat << EOF > "$TMP/expected_no_id.txt"
fail: $(realpath "$TMP")/instance.json
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "" (line 1, column 1)
    at evaluate path "/type"
EOF

diff "$TMP/output_no_id.txt" "$TMP/expected_no_id.txt"

# JSON error without $id
cat << 'EOF' | "$1" validate - "$TMP/instance.json" --json >"$TMP/stdout_no_id.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected_no_id.txt"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/type",
      "absoluteKeywordLocation": "file://$CWD#/type",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 3 ],
      "error": "The value was expected to be of type string but it was of type integer"
    }
  ]
}
EOF

diff "$TMP/stdout_no_id.txt" "$TMP/expected_no_id.txt"
