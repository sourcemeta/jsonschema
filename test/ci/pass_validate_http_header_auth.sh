#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5893

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
    "$id": "http://localhost:5893/schema",
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

cat << EOF > "$TMP/schema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$ref": "http://localhost:${PORT}/schema"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --http \
  --header "Authorization: Bearer secret" \
  > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
