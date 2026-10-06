#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5899

# A server that accepts the connection and never answers, which is the one
# failure the caller can do something about by waiting longer
cat << 'EOF' > "$TMP/server.js"
const http = require('http');
const server = http.createServer(() => {});
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
  --url "http://localhost:${PORT}/v1/chat/completions" --model my-model \
  --timeout 2 \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Unexpected error
test "$EXIT_CODE" = "1"

cat << EOF > "$TMP/expected.txt"
error: The completion request did not answer within 2 seconds
  at url http://localhost:${PORT}/v1/chat/completions

Pass --timeout/-T with a larger number of seconds to wait longer
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" llm "$TMP/schema.json" --ask hello \
  --url "http://localhost:${PORT}/v1/chat/completions" --model my-model \
  --timeout 2 --json \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Unexpected error
test "$EXIT_CODE" = "1"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "The completion request did not answer within 2 seconds",
  "url": "http://localhost:${PORT}/v1/chat/completions"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
