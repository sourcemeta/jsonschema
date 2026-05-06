#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5891

cat << 'EOF' > "$TMP/server.js"
const http = require('http');
const server = http.createServer((req, res) => {
  if (req.headers['authorization'] !== 'Bearer secret') {
    res.statusCode = 401;
    res.end('unauthorized');
    return;
  }
  res.setHeader('content-type', 'application/json');
  res.end(JSON.stringify({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "$id": "http://localhost:5891/schema",
    "type": "string"
  }));
});
server.listen(parseInt(process.argv[2], 10));
EOF

node "$TMP/server.js" "$PORT" &
SERVER_PID="$!"

clean() {
  kill "$SERVER_PID" 2>/dev/null || true
  rm -rf "$TMP"
}
trap clean EXIT

sleep 2

mkdir "$TMP/project"

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "http://localhost:${PORT}/schema": "./vendor/schema.json"
  }
}
EOF

cd "$TMP/project"

"$1" install \
  > "$TMP/output_no_auth.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected_no_auth.txt"
Fetching       : http://localhost:${PORT}/schema
error: Failed to fetch schema
  at uri http://localhost:${PORT}/schema
EOF

diff "$TMP/output_no_auth.txt" "$TMP/expected_no_auth.txt"

rm -f "$TMP/project/jsonschema.lock.json"

"$1" install --header "Authorization: Bearer secret" \
  > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : http://localhost:${PORT}/schema
Installed      : $(realpath "$TMP")/project/vendor/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_schema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "http://localhost:${PORT}/schema",
  "type": "string"
}
EOF

diff "$TMP/project/vendor/schema.json" "$TMP/expected_schema.json"

HASH="$(shasum -a 256 < "$TMP/project/vendor/schema.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "http://localhost:${PORT}/schema": {
      "path": "./vendor/schema.json",
      "hash": "${HASH}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
