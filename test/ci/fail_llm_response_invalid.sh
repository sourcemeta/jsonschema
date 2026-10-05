#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5896

# A provider that took the schema, answered with JSON, and produced a document
# the schema rejects. This is the finding the command exists to report, so the
# document comes out alongside the errors and the coordinates point into it
cat << 'EOF' > "$TMP/server.js"
const http = require('http');
const server = http.createServer((request, response) => {
  response.setHeader('content-type', 'application/json');
  response.end(JSON.stringify({
    choices: [ {
      finish_reason: "stop",
      message: { content: "{\n  \"capital\": 42\n}", role: "assistant" }
    } ]
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
  "type": "object",
  "properties": { "capital": { "type": "string" } },
  "required": [ "capital" ],
  "additionalProperties": false
}
EOF

"$1" llm "$TMP/schema.json" --ask "What is the capital of Germany?" \
  --url "http://localhost:${PORT}/v1/chat/completions" --model my-model \
  > "$TMP/output.txt" 2> "$TMP/error.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
{
  "capital": 42
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_error.txt"

fail: http://localhost:${PORT}/v1/chat/completions
error: The generated document does not conform to the schema
  The value was expected to be of type string but it was of type integer
    at instance location "/capital" (line 2, column 3)
    at evaluate path "/properties/capital/type"
  The object value was expected to validate against the defined properties subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/error.txt" "$TMP/expected_error.txt"
