#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

if command -v nproc > /dev/null 2>&1
then
  CORES="$(nproc)"
else
  CORES="$(sysctl -n hw.ncpu)"
fi

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "Invalid type",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose \
  1> "$TMP/stdout.txt" 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected_stdout.txt"
$(realpath "$TMP")/test.json:
  1/2 PASS First test
  2/2 PASS Invalid type
EOF

cat << EOF > "$TMP/expected_stderr.txt"
Using parallelism: $CORES
EOF

diff "$TMP/stdout.txt" "$TMP/expected_stdout.txt"
diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"

JOBS="$((CORES + 1))"

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose --jobs "$JOBS" \
  1> "$TMP/stdout.txt" 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected_stderr.txt"
Using parallelism: $JOBS
EOF

diff "$TMP/stdout.txt" "$TMP/expected_stdout.txt"
diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" \
  1> "$TMP/stdout.txt" 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected_stdout.txt"
$(realpath "$TMP")/test.json: PASS 2/2
EOF

cat << 'EOF' > "$TMP/expected_stderr.txt"
EOF

diff "$TMP/stdout.txt" "$TMP/expected_stdout.txt"
diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"
