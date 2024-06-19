#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "description": "My sample suite",
  "schema": "http://json-schema.org/draft-04/schema#",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": {}
    },
    {
      "description": "Invalid type, expected to fail",
      "valid": true,
      "data": { "type": 1 }
    }
  ]
}
EOF

"$1" test "$TMP/test.json" 1> "$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
fi

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json
    My sample suite - First test
        PASS
    My sample suite - Invalid type, expected to fail
        FAIL

error: The target document is expected to be one of the given values
  at instance location "/type"
  at evaluate path "/properties/type/anyOf/0/\$ref/enum"
error: Mark the current position of the evaluation process for future jumps
  at instance location "/type"
  at evaluate path "/properties/type/anyOf/0/\$ref"
error: The target document is expected to be of the given type
  at instance location "/type"
  at evaluate path "/properties/type/anyOf/1/type"
error: The target is expected to match at least one of the given assertions
  at instance location "/type"
  at evaluate path "/properties/type/anyOf"
error: The target is expected to match all of the given assertions
  at instance location ""
  at evaluate path "/properties"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
echo "PASS" 1>&2
