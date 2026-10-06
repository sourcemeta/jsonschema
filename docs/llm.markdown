LLM Structured Outputs
======================

```sh
jsonschema llm <schema.json|.yaml> --ask/-a <prompt>
  --url/-u <completion-url> --model/-m <model>
  [--param/-p <pointer>=<value>] [--header/-H "<name>: <value>"]
  [--upgrade/-U draft4|draft6|draft7|2019-09|2020-12]
  [--timeout/-T <seconds>] [--raw/-R] [--dry-run/-D] [--trace/-t]
  [--without-id/-w] [--resolve/-r <schemas-or-directories> ...]
  [--default-dialect/-d <uri>] [--configuration/-C <path>]
  [--verbose/-v] [--debug/-g] [--json/-j] [--color auto|always|never]
```

Use this command to make a model answer with a JSON document that conforms to a
schema you supply, instead of prose you have to parse afterwards. In a pipeline
that is the difference between a contract and a guess. A renamed field, an
absent one, or a number arriving as a string is a crash downstream, or a
silently bad write.

> [!NOTE]
> The whole field converged on JSON Schema to write these contracts in,
> independently and for the same reasons. To learn more about the role JSON
> Schema plays in AI, see [The only schema language AI speaks is JSON
> Schema](https://www.sourcemeta.com/blog/ai-only-speaks-json-schema/).

Sending a single self-contained schema by hand is easy. Real ones are rarely so
tidy. Yours may reference other schemas, and no provider fetches anything for
you. It may be written in YAML, which no provider reads. It may sit on an older
dialect than the provider recognises. This command handles all three.
References are bundled in, YAML becomes JSON, and keywords are respelled as
whichever dialect you ask for, so a Draft 4 schema in YAML spread over half a
dozen files works as it stands. Whatever comes back is then validated against
the schema you wrote, not the prepared copy that went out, so anything the
provider failed to honour still shows up instead of slipping through.

> [!NOTE]
> See [Resolving External References](./guides/resolution.markdown) for every way of
> making referenced schemas available, including how to handle a reference whose
> URI differs from the identifier the target schema declares.

Providers
---------

*JSON Schema compliance varies a lot between providers*. How much of the
language an engine enforces, how strictly, and which dialects it recognises all
differ, sometimes between models of one provider. Treat the sections below as
a starting point and check the documentation of whichever provider you use.

This command speaks one request shape, the OpenAI Chat Completions
`response_format` with a nested `json_schema`, so any endpoint that takes it
works without special casing. Support for other shapes, Anthropic's among them,
is on the way.

**This landscape changes constantly. If this command stops working against a
provider, please open an issue at
https://github.com/sourcemeta/jsonschema/issues so we can keep up.**

### dottxt on Doubleword

This is the one we recommend, and the most JSON Schema compliant implementation
we are aware of so far. [.txt](https://dottxt.ai) built its generation engine
around JSON Schema rather than bolting validation on afterwards, which it
describes in [How JSON Schema makes LLM output
reliable](https://json-schema.org/blog/posts/dottxt-case-study). Its models are
served through [Doubleword](https://doubleword.ai):

```sh
jsonschema llm path/to/schema.json \
  --ask "What is the capital of Germany?" \
  --url https://api.doubleword.ai/v1/chat/completions \
  --model Qwen/Qwen3.5-35B-A3B-FP8-dottxt \
  --header "Authorization: Bearer $DOUBLEWORD_API_KEY"
```

Doubleword can sometimes be slow to serve a request, independently of the
model, which is what the generous default `--timeout/-T` leaves room for.

### OpenAI

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.openai.com/v1/chat/completions \
  --model "$OPENAI_MODEL" \
  --header "Authorization: Bearer $OPENAI_API_KEY"
```

At least at the time of this writing, they place extra restrictions on the input
schema beyond what JSON Schema itself asks for. Their [Structured
Outputs](https://developers.openai.com/api/docs/guides/structured-outputs)
documentation is the place to check what those are.

### OpenAI-compatible servers

That covers [vLLM](https://docs.vllm.ai/en/latest/serving/openai_compatible_server.html),
[llama.cpp](https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md),
[LM Studio](https://lmstudio.ai/docs/app/api/endpoints/openai),
[Ollama's compatibility endpoint](https://docs.ollama.com/openai) and gateways
such as [OpenRouter](https://openrouter.ai/docs/features/structured-outputs).
Only the URL changes:

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url http://localhost:1234/v1/chat/completions --model my-model
```

Watch the token-limit field, which these often disagree on. Set it with
`--param/-p`, which writes into the request body at the JSON Pointer you name:
OpenAI takes `/max_completion_tokens` while Ollama normalises to `/max_tokens`.
For example:

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url http://localhost:11434/v1/chat/completions --model my-model \
  --param /max_tokens=2048
```

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
