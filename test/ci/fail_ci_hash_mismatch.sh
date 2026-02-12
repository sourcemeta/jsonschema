#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": "./vendor/schema.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output_install.txt" 2>&1

cat << EOF > "$TMP/expected_install.txt"
Fetching       : file://$(realpath "$TMP")/source/schema.json
Installed      : $(realpath "$TMP")/project/vendor/schema.json
EOF

diff "$TMP/output_install.txt" "$TMP/expected_install.txt"

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

echo "TAMPERED CONTENT" > "$TMP/project/vendor/schema.json"

"$1" ci > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Mismatched     : $(realpath "$TMP")/project/vendor/schema.json
error: File hash does not match lock file in frozen mode
  at uri file://$(realpath "$TMP")/source/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

"$1" ci --json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "events": [
    {
      "type": "mismatched",
      "path": "$(realpath "$TMP")/project/vendor/schema.json"
    },
    {
      "type": "error",
      "uri": "file://$(realpath "$TMP")/source/schema.json",
      "message": "File hash does not match lock file in frozen mode"
    }
  ]
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"
