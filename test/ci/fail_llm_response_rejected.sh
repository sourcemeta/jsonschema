#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5897

# Error bodies are not portable, not even within one provider, and one of the
# shapes seen in the wild is not JSON at all. So each body comes out as it came
# in, and what the status says about whose problem it is decides the exit code
cat << 'EOF' > "$TMP/server.js"
const http = require('http');
const server = http.createServer((request, response) => {
  if (request.url === '/reject') {
    response.statusCode = 400;
    response.setHeader('content-type', 'application/json');
    response.end(JSON.stringify({ message: "Bad Request", type: "Bad Request", code: 400 }));
    return;
  }

  response.statusCode = 403;
  response.setHeader('content-type', 'text/plain');
  response.end("Real-time access to 'my-model' is blocked by a routing rule.");
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

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" llm "$TMP/schema.json" --ask hello \
  --url "http://localhost:${PORT}/reject" --model my-model \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3"

cat << EOF > "$TMP/expected.txt"
error: Unsuccessful HTTP response
  with status 400 Bad Request
  at url http://localhost:${PORT}/reject

{"message":"Bad Request","type":"Bad Request","code":400}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" llm "$TMP/schema.json" --ask hello \
  --url "http://localhost:${PORT}/blocked" --model my-model \
  > "$TMP/output_blocked.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected_blocked.txt"
error: Unsuccessful HTTP response
  with status 403 Forbidden
  at url http://localhost:${PORT}/blocked

Real-time access to 'my-model' is blocked by a routing rule.
EOF

diff "$TMP/output_blocked.txt" "$TMP/expected_blocked.txt"
