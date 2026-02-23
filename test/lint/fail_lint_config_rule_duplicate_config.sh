#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule1.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "my_rule",
  "description": "First rule",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/rule2.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "my_rule",
  "description": "Second rule",
  "required": [ "description" ]
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "rules": [
      "./rule1.json",
      "./rule2.json"
    ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected.txt"
error: A lint rule with this name already exists
  at file path $(realpath "$TMP")/rule2.json
  at rule my_rule
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cd "$TMP"
"$1" lint --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "A lint rule with this name already exists",
  "filePath": "$(realpath "$TMP")/rule2.json",
  "rule": "my_rule"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
