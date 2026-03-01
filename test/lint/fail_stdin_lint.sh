#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" lint - >"$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "enum": [ "foo" ]
}
EOF
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
<stdin>:6:3:
  Setting `type` alongside `enum` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/enum"
<stdin>:5:3:
  Setting `type` alongside `enum` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at location "/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
cat << 'EOF' | "$1" lint - --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "enum": [ "foo" ]
}
EOF
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "valid": false,
  "health": 0,
  "errors": [
    {
      "path": "<stdin>",
      "id": "enum_with_type",
      "message": "Setting `type` alongside `enum` is considered an anti-pattern, as the enumeration choices already imply their respective types",
      "description": null,
      "schemaLocation": "/type",
      "position": [ 5, 3, 5, 18 ]
    },
    {
      "path": "<stdin>",
      "id": "enum_with_type",
      "message": "Setting `type` alongside `enum` is considered an anti-pattern, as the enumeration choices already imply their respective types",
      "description": null,
      "schemaLocation": "/enum",
      "position": [ 6, 3, 6, 19 ]
    }
  ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
