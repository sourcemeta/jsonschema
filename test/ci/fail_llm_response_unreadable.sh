#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5898

# Two ways a response can succeed and still say nothing: an envelope with no
# generated document in it, and a generated document that is not JSON. The first
# is a gap in what this command knows, the second is a finding about the provider
cat << 'EOF' > "$TMP/server.js"
const http = require('http');
const server = http.createServer((request, response) => {
  response.setHeader('content-type', 'application/json');
  if (request.url === '/envelope') {
    response.end(JSON.stringify({ id: "resp_1", object: "chat.completion" }));
    return;
  }

  response.end(JSON.stringify({
    choices: [ { finish_reason: "stop", message: { content: "Berlin, obviously.", role: "assistant" } } ]
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

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" llm "$TMP/schema.json" --ask hello \
  --url "http://localhost:${PORT}/envelope" --model my-model \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3"

cat << EOF > "$TMP/expected.txt"
error: The response does not carry a generated document
  with status 200 OK
  at url http://localhost:${PORT}/envelope

The provider responded with:

{
  "id": "resp_1",
  "object": "chat.completion"
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" llm "$TMP/schema.json" --ask hello \
  --url "http://localhost:${PORT}/prose" --model my-model \
  > "$TMP/output_prose.txt" 2> "$TMP/error_prose.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected_prose.txt"
Berlin, obviously.
EOF

diff "$TMP/output_prose.txt" "$TMP/expected_prose.txt"

cat << EOF > "$TMP/expected_error_prose.txt"

fail: http://localhost:${PORT}/prose
error: The generated document is not valid JSON
  Failed to parse the JSON document
    at line 1, column 1
EOF

diff "$TMP/error_prose.txt" "$TMP/expected_error_prose.txt"
