#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5892

cat << 'EOF' > "$TMP/server.js"
const http = require('http');
const server = http.createServer((req, res) => {
  res.setHeader('content-type', 'application/json');
  res.end(JSON.stringify({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "$id": "http://localhost:5892/schema",
    "type": "string",
    "x-received-authorization": req.headers['authorization'] || '',
    "x-received-x-tenant": req.headers['x-tenant'] || ''
  }, null, 2));
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
  --header "Authorization: Bearer secret" \
  --header "X-Tenant: acme" \
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
  "type": "string",
  "x-received-authorization": "Bearer secret",
  "x-received-x-tenant": "acme"
}
EOF

diff "$TMP/project/vendor/schema.json" "$TMP/expected_schema.json"
