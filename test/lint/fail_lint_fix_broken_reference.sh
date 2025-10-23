#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "properties": {
    "foo": {
      "$ref": "#/definitions/object",
      "additionalProperties": {
        "type": "string"
      }
    },
    "bar": {
      "$ref": "#/properties/foo/additionalProperties"
    }
  },
  "definitions": {
    "object": {
      "type": "object"
    }
  }
}
EOF

cp "$TMP/schema.json" "$TMP/original.json"

cd "$TMP"
"$1" lint "$TMP/schema.json" --fix >"$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Could not autofix the schema without breaking its internal references
  at file path $(realpath "$TMP")/schema.json
  at location "/properties/bar/\$ref"

This is an unexpected error, as making the auto-fix functionality work in all
cases is tricky. We are working hard to improve the auto-fixing functionality
to handle all possible edge cases, but for now, try again without \`--fix/-f\`
and apply the suggestions by hand.

Also consider consider reporting this problematic case to the issue tracker,
so we can add it to the test suite and fix it:

https://github.com/sourcemeta/jsonschema/issues
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
# The schema must have not being modified
diff "$TMP/schema.json" "$TMP/original.json"
