#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
  "allOf": [
    { "$ref": "#/$defs/not-a-schema" }
  ],
  "$defs": {
    "not-a-schema": {
      "type": "string"
    }
  },
  "default": "test"
}
EOF

"$1" lint "$TMP/schema.json" --fix 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
.
error: Could not autofix the schema without breaking its internal references
  at file path $(realpath "$TMP")/schema.json
  at location "/allOf/0/\$ref"

This is an unexpected error, as making the auto-fix functionality work in all
cases is tricky. We are working hard to improve the auto-fixing functionality
to handle all possible edge cases, but for now, try again without \`--fix/-f\`
and apply the suggestions by hand.

Also consider consider reporting this problematic case to the issue tracker,
so we can add it to the test suite and fix it:

https://github.com/sourcemeta/jsonschema/issues
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" lint "$TMP/schema.json" --fix --json >"$TMP/stdout.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "Could not autofix the schema without breaking its internal references",
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/allOf/0/\$ref"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
