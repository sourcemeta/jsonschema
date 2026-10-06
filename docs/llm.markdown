LLM Structured Outputs
======================

```sh
jsonschema llm <schema.json|.yaml> --ask/-A <prompt>
  --url/-u <completion-url> --model/-M <model>
  [--param/-P <pointer>=<value>] [--header/-H "<name>: <value>"]
  [--upgrade/-U draft4|draft6|draft7|2019-09|2020-12]
  [--timeout/-T <seconds>] [--raw/-R] [--dry-run/-D] [--trace/-t]
  [--without-id/-w] [--resolve/-r <schemas-or-directories> ...]
  [--default-dialect/-d <uri>] [--configuration/-C <path>]
  [--verbose/-v] [--debug/-g] [--json/-j] [--color auto|always|never]
```

> [!NOTE]
> See [Resolving External References](./guides/resolution.markdown) for every way of
> making referenced schemas available, including how to handle a reference whose
> URI differs from the identifier the target schema declares.

A schema is usually a gate you put in front of data that already exists.
Structured outputs turn it around: the provider restricts what the model is
allowed to emit, token by token, so the only thing it can produce is a document
matching the schema you handed over. The schema stops being a check and becomes
the question. You describe the shape of the answer you want, and you get that
shape back instead of prose to pick apart afterwards.

This is why JSON Schema ended up as the contract language of the entire AI
stack, a story we tell in [The only schema language AI speaks is JSON
Schema](https://www.sourcemeta.com/blog/ai-only-speaks-json-schema/). Every
major provider converged on it independently, so the same schema you already use
to validate your data is the artifact that constrains a model, describes a tool
to an agent, and travels over the Model Context Protocol.

The catch is that the guarantee is only as good as each provider's
implementation of it, and a schema that one engine enforces exactly may be a
vague hint to the next. This command sends your schema to a completion endpoint,
reads the document that comes back, and validates it against **the schema you
wrote** rather than against whatever was sent after preparation. A provider that
quietly drops a constraint therefore shows up as a validation failure, exit code
2, with the document and the errors side by side.

```sh
jsonschema llm path/to/schema.json \
  --ask "What is the capital of Germany?" \
  --url https://api.openai.com/v1/chat/completions \
  --model "$OPENAI_MODEL" \
  --header "Authorization: Bearer $OPENAI_API_KEY"
```

The generated document goes to standard output exactly as the model emitted it,
and everything else to standard error, so redirecting leaves the document alone
and ready to pipe:

```
{
  "capital": "Berlin"
}

tokens: 17 prompt, 128 completion, 145 total
```

When it does not conform, the document is still printed, because the coordinates
are useless without it:

```
{
  "capital": 42
}

fail: https://api.openai.com/v1/chat/completions
error: The generated document does not conform to the schema
  The value was expected to be of type string but it was of type integer
    at instance location "/capital" (line 2, column 3)
    at evaluate path "/properties/capital/type"
```

Building the request, extracting the answer out of whatever envelope it arrives
in and keeping a chain of thought from being mistaken for it are all handled for
you. Pass the credential with `--header/-H`, anything else a given endpoint wants
in the request body with `--param/-P`, and `--dry-run/-D` to print the request
instead of sending it. A boolean schema, an OpenAPI description and a schema that
declares a custom meta-schema are each refused, with the reason.

No provider reads the `$schema` a document declares. The keyword appears nowhere
in any provider's structured-output documentation, so what varies between
dialects is purely how the keywords are spelled, and spelling is what these
engines match on. Schemas go out as 2020-12 by default. Use `--upgrade/-U` to
spell one as another dialect instead, since an engine that chokes on `$defs` may
accept `definitions` for the identical schema.

Providers
---------

What each provider supports, and which dialect it expects, varies between them
and shifts over time. Treat the sections below as a starting point and check your
provider's own structured-output documentation for the subset it accepts. Some
accept only `{ "type": "json_object" }`, which guarantees parseable JSON while
ignoring the schema entirely, and a gateway that routes to several upstreams may
enforce the schema for some and treat it as a hint for others.

**This landscape changes constantly. If this command stops working against a
provider, please open an issue at
https://github.com/sourcemeta/jsonschema/issues so we can keep up.**

### dottxt on Doubleword

[.txt](https://dottxt.ai) built its generation engine around JSON Schema rather
than bolting validation on afterwards, which it describes in [How JSON Schema
makes LLM output
reliable](https://json-schema.org/blog/posts/dottxt-case-study). Its models are
served through [Doubleword](https://doubleword.ai):

```sh
jsonschema llm path/to/schema.json \
  --ask "What is the capital of Germany?" \
  --url https://api.doubleword.ai/v1/chat/completions \
  --model Qwen/Qwen3.5-35B-A3B-FP8-dottxt \
  --header "Authorization: Bearer $DOUBLEWORD_API_KEY" \
  --without-id
```

Compiling a schema into a grammar takes this engine well past the time an
ordinary completion takes, which is what the generous default `--timeout/-T` is
for. An involved schema may want more still.

### OpenAI

The request this command sends is OpenAI's own shape, so it goes unmodified:

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.openai.com/v1/chat/completions \
  --model "$OPENAI_MODEL" \
  --header "Authorization: Bearer $OPENAI_API_KEY"
```

Their Structured Outputs requires `additionalProperties: false` on every object,
every property listed in `required`, and an object at the root.

### OpenAI-compatible servers

Anything implementing the same endpoint works without special casing, which
covers vLLM, llama.cpp, LM Studio, Ollama's compatibility endpoint and gateways
such as OpenRouter. Only the URL changes:

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url http://localhost:1234/v1/chat/completions --model my-model
```

Watch the token-limit field, which these disagree on: OpenAI takes
`max_completion_tokens` while Ollama normalises to `max_tokens`.

### Anthropic

Not reachable through this command today, for two separate reasons.

Their [OpenAI SDK compatibility
layer](https://platform.claude.com/docs/en/cli-sdks-libraries/libraries/openai-sdk)
accepts the endpoint but documents `response_format` as ignored. The schema would
travel and constrain nothing, so any non-conformance reported here would be an
artifact of the compatibility layer rather than anything about the model.

Their native [Structured
Outputs](https://platform.claude.com/docs/en/build-with-claude/structured-outputs)
does enforce the schema, but on `/v1/messages` with the schema carried at
`output_config.format.schema`, a different request shape than the one this
command speaks.

### Google Gemini

Also not reachable. Its [OpenAI compatibility
endpoint](https://ai.google.dev/gemini-api/docs/openai) routes structured
outputs through a client-side parse helper rather than through `response_format`
with a nested `json_schema`, and its native API carries the schema under
`generationConfig` instead.

Examples
--------

### Ask a model and check what comes back

```sh
jsonschema llm path/to/schema.json \
  --ask "What is the capital of Germany?" \
  --url https://api.example.com/v1/chat/completions \
  --model my-model \
  --header "Authorization: Bearer $TOKEN"
```

### Print the request without sending it

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --dry-run
```

### Set a token limit

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --param /max_completion_tokens=2048
```

### Ask a local model

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url http://localhost:1234/v1/chat/completions --model my-model
```

### Spell the schema as a dialect other than the default

```sh
jsonschema llm path/to/draft4-schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --upgrade draft7
```

### Get the result in a machine-readable form

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --json
```

### Keep a failing result for later

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  > document.json 2> errors.txt
```

### Ask about a schema that references other schemas

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --resolve path/to/imported.json
```

### Ask about a schema piped from standard input

```sh
cat path/to/schema.json | jsonschema llm - --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model
```

### Capture a response envelope without reading it

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --raw > envelope.json
```
