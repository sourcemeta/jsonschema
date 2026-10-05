#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5894

cat << 'EOF' > "$TMP/server.js"
const http = require('http');
const server = http.createServer((request, response) => {
  response.setHeader('content-type', 'application/json');
  response.end(JSON.stringify({
    choices: [ {
      finish_reason: "stop",
      index: 0,
      message: {
        content: "{\n  \"capital\": \"Berlin\"\n}",
        reasoning_content: "The capital of Germany is Berlin.",
        refusal: null,
        role: "assistant"
      }
    } ],
    created: 1791216234,
    id: "resp_eb4a62d3-39eb-42fd-9231-c6650b2a0668",
    object: "chat.completion",
    usage: {
      completion_tokens: 128,
      completion_tokens_details: { reasoning_tokens: 116 },
      prompt_tokens: 17,
      total_tokens: 145
    }
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
  > "$TMP/output.txt" 2> "$TMP/error.txt"

cat << 'EOF' > "$TMP/expected.txt"
{
  "capital": "Berlin"
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected_error.txt"
finish reason: stop
tokens: 17 prompt, 128 completion, 145 total
EOF

diff "$TMP/error.txt" "$TMP/expected_error.txt"

cat << 'EOF' > "$TMP/plain.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
EOF

"$1" llm "$TMP/plain.json" --ask "What is the capital of Germany?" \
  --url "http://localhost:${PORT}/v1/chat/completions" --model my-model --json \
  > "$TMP/output_json.txt" 2>&1

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "valid": true,
  "raw": "{\n  \"capital\": \"Berlin\"\n}",
  "document": {
    "capital": "Berlin"
  },
  "output": {
    "valid": true
  },
  "finishReason": "stop",
  "usage": {
    "promptTokens": 17,
    "completionTokens": 128,
    "totalTokens": 145
  }
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
