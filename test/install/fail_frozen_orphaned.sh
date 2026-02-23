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

cat << 'EOF' > "$TMP/source/other.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "number"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": "./vendor/schema.json",
    "file://$(realpath "$TMP")/source/other.json": "./vendor/other.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output_install.txt" 2>&1

cat << EOF > "$TMP/expected_install.txt"
Fetching       : file://$(realpath "$TMP")/source/other.json
Installed      : $(realpath "$TMP")/project/vendor/other.json
Fetching       : file://$(realpath "$TMP")/source/schema.json
Installed      : $(realpath "$TMP")/project/vendor/schema.json
EOF

diff "$TMP/output_install.txt" "$TMP/expected_install.txt"

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": "./vendor/schema.json"
  }
}
EOF

"$1" install --frozen > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Integrity verification failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
Up to date     : file://$(realpath "$TMP")/source/schema.json
Orphaned       : file://$(realpath "$TMP")/source/other.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

"$1" install --frozen --json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Integrity verification failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "events": [
    {
      "type": "up-to-date",
      "uri": "file://$(realpath "$TMP")/source/schema.json"
    },
    {
      "type": "orphaned",
      "uri": "file://$(realpath "$TMP")/source/other.json"
    }
  ]
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"
