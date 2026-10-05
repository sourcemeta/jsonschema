Asking a Model
==============

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

Constrained generation is a provider claiming that what a model emits will
conform to a schema you hand it. This command tests that claim. It sends the
schema to a completion endpoint along with a prompt, reads the document that
comes back, and validates that document against the schema. Whether the provider
honoured the schema is the answer, and the exit code is how you get it.

The point is the asymmetry. Preparing a schema for a provider can lose
constraints, so the response is validated against **the schema you wrote**
rather than against whatever went on the wire. When the two disagree, the
command has found something.

Any endpoint that accepts an OpenAI Chat Completions `response_format` with a
nested `json_schema` object works, which is a single widely-cloned shape rather
than a list of vendors. No vendor name appears anywhere in the interface.

```sh
jsonschema llm path/to/schema.json \
  --ask "What is the capital of Germany?" \
  --url https://api.openai.com/v1/chat/completions \
  --model my-model \
  --header "Authorization: Bearer $OPENAI_API_KEY"
```

On success the generated document goes to standard output exactly as the model
emitted it, and the command exits 0:

```json
{
  "capital": "Berlin"
}
```

When the document does not conform, it still goes to standard output, the
validation errors go to standard error, and the command exits 2:

```
{
  "capital": 42
}
fail: https://api.openai.com/v1/chat/completions
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "/capital" (line 2, column 3)
    at evaluate path "/properties/capital/type"
```

The document is printed because the coordinates are useless without it. It
arrived over the network, so there is no file to open, and the line and column
refer to exactly the bytes that were printed. They are computed against the
string the model emitted rather than against a reformatted copy, so they point
at what it literally wrote.

Exit Codes
----------

| Outcome | Code |
| --- | --- |
| The response validates | 0 |
| The response does not validate, is not JSON, or carries more than one document | 2 |
| The provider refused what was sent (400, 413, 422), or the response cannot be read | 3 |
| A bad option, a bad parameter, or a missing one | 5 |
| The credential, the URL or the model does not work (401, 403, 404) | 6 |
| The provider is unwell or busy (429, 5xx), or the request never arrived | 1 |

Code 2 is the interesting one. It means the provider accepted the schema,
returned JSON, and the JSON does not conform. That makes this command a harness
on its own, with no wrapper script.

Request Body
------------

One fixed shape goes out:

```json
{
  "model": "<--model>",
  "messages": [ { "role": "user", "content": "<--ask>" } ],
  "response_format": {
    "type": "json_schema",
    "json_schema": {
      "name": "schema",
      "strict": true,
      "schema": { "<the prepared schema>": "..." }
    }
  }
}
```

Nothing else is sent. No temperature, no token limit, no sampling settings. Each
of those is named differently by each provider and several refuse a non-default
value, so the portable request omits them and `--param/-P` adds what a given
endpoint wants.

`strict` is in the template because OpenAI's Structured Outputs does not engage
at all without it. Turn it off with
`--param /response_format/json_schema/strict=false`.

### Parameters

`--param/-P` takes a JSON Pointer and a value, writing into the request body at
that location and creating the objects along the way:

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url http://localhost:8080/v1/chat/completions --model my-model \
  --param /max_completion_tokens=2048 \
  --param /provider/require_parameters=true
```

Values are read as JSON and fall back to the text as written, so `=true` is a
boolean, `=2048` a number, `=null` null and `=my-model` a string. Without that
rule `strict=false` would arrive as the string `"false"`, which is truthy to most
JSON consumers.

Where a setting lives decides how you set it:

| Where it lives | How to set it |
| --- | --- |
| In the request body | `--param/-P <pointer>=<value>` |
| In the request headers | `--header/-H "<name>: <value>"` |
| Never leaves your machine | an option of its own |

That is why `--timeout/-T` is an option rather than a parameter. A
`--param /timeout=30` would POST `{"timeout": 30}` as a field the provider does
not know, to be refused or quietly ignored while the hang it was meant to prevent
happens anyway. The default is 120 seconds, which is above the usual HTTP default
because compiling a grammar out of a schema takes a provider well past what an
ordinary request takes.

Dialects
--------

No provider reads the `$schema` a document declares. Across every provider's
structured-output documentation the keyword does not appear once, so there is no
dialect negotiation anywhere in this ecosystem. Each provider has one hardcoded
set of keywords it recognises, and the names of those keywords are the only thing
that matters.

So what varies between dialects is purely how the keywords are spelled, and
spelling is what these compilers match on. By default the schema goes out spelled
as the latest dialect, 2020-12, since those are the names most widely recognised.
A Draft 7 schema is therefore sent with `$defs` rather than the `definitions` it
was written with.

Use `--upgrade/-U` to pick another dialect instead. A provider that chokes on
`$defs` may accept `definitions` for the identical schema, which makes this the
one real lever over what a provider recognises:

```sh
jsonschema llm path/to/draft4-schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --upgrade draft7
```

> [!NOTE]
> As in the `upgrade` command, this means "spell it as at least this dialect". A
> schema already at or beyond the target is left alone, so this cannot move a
> 2020-12 schema back onto Draft 7.

> [!IMPORTANT]
> A schema that declares a custom meta-schema is refused outright. A provider
> recognises one fixed set of keywords and reads no dialect declaration, so the
> keywords a custom meta-schema describes are ones it cannot know about and would
> silently ignore, which is the opposite of what asking a model to honour a schema
> is for. Rewrite the schema against an official dialect, using only official
> vocabularies.

Credentials and Network Access
------------------------------

Pass the credential with `--header/-H`, the same way the `install` and `validate`
commands do:

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --header "Authorization: Bearer $TOKEN"
```

Headers are held in wiping storage so that a credential one carries is never
retained in an ordinary string, and the request does not follow redirects, so a
credential is never carried to a host other than the one you named.

> [!IMPORTANT]
> This command does not accept `--http/-h`. Remote reference resolution would
> send these headers, the credential among them, to whatever host a reference
> names. Keeping the two apart is what makes `--header/-H` unambiguously the
> completion request's headers.

Authenticated remote schemas compose through the command that already fetches
them:

```sh
jsonschema bundle path/to/schema.json --http \
  --header "Authorization: Bearer $SCHEMA_TOKEN" > bundled.json

jsonschema llm bundled.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --header "Authorization: Bearer $PROVIDER_TOKEN"
```

Inspecting the Request
----------------------

`--dry-run/-D` runs everything up to the socket and prints the request instead
of sending it. No network, no credential, no cost:

```sh
jsonschema llm path/to/schema.json --ask "What is the capital of Germany?" \
  --url https://api.example.com/v1/chat/completions --model my-model --dry-run
```

```
POST https://api.example.com/v1/chat/completions
Content-Type: application/json

{
  "model": "my-model",
  "messages": [
    {
      "role": "user",
      "content": "What is the capital of Germany?"
    }
  ],
  "response_format": {
    "type": "json_schema",
    "json_schema": {
      "name": "schema",
      "strict": true,
      "schema": {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": "file:///path/to/schema.json",
        "type": "object",
        "required": [ "capital" ],
        "properties": {
          "capital": {
            "type": "string"
          }
        },
        "additionalProperties": false
      }
    }
  }
}
```

Because it runs the whole pipeline, it doubles as a check on an invocation,
reporting an unresolvable reference, a parameter that names nowhere, or a header
without a colon with the exit code each would really produce.

> [!NOTE]
> Bundling gives a schema that declares no identifier of its own the one it was
> read under, which for a local file is a `file://` URI naming where it sits. That
> identifier goes out with the schema, the same way the `bundle` command writes
> it. Pass `--without-id/-w` to remove identifiers before sending.

Header names and their order are shown while their values are held back, so that
printing a request never puts a live credential into output you paste into a bug
report.

Reading the Response
--------------------

Exactly three things are read out of the response envelope:

| What | Where |
| --- | --- |
| The generated document | `choices[0].message.content` |
| How generation stopped | `choices[0].finish_reason` |
| Token counts, best effort | `usage.prompt_tokens`, `completion_tokens`, `total_tokens` |

Token counts are read where a provider reports them and skipped where it does
not, as the validation result is the point and a count never decides it.

> [!WARNING]
> A reasoning model puts its chain of thought in `message.reasoning_content`,
> right beside the answer, and it can be most of the tokens a response spends.
> That field is never read as output.

A response that succeeds but carries no generated document is a gap in what this
command knows rather than a mistake you made, so the body comes out unchanged and
the command exits 3. Error bodies get the same treatment: they are not portable,
not even within one provider, and at least one provider returns a body that is
not JSON at all, so none of them is parsed, reformatted or summarised.

Use `--raw/-R` to print the response envelope without reading it at all, which is
how you capture one to keep.

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

### Set a token limit, which each provider names differently

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.openai.com/v1/chat/completions --model my-model \
  --param /max_completion_tokens=2048
```

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url http://localhost:11434/v1/chat/completions --model my-model \
  --param /max_tokens=2048
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

### Remove schema identifiers before sending

```sh
jsonschema llm path/to/schema.json --ask "..." \
  --url https://api.example.com/v1/chat/completions --model my-model \
  --without-id
```

Providers That Cannot Be Tested This Way
----------------------------------------

Some providers accept only `{ "type": "json_object" }`, which guarantees
parseable JSON and ignores the schema entirely. Their own documentation puts the
schema into the prompt as text and validates client-side afterwards. There is no
constraint for this command to audit, so sending it a schema is expected either
to fail or to be quietly downgraded to unconstrained JSON.

Providers whose native API takes a different shape altogether are reachable only
through their OpenAI compatibility endpoint, where they have one. The native
endpoint is not a target of this command.

> [!NOTE]
> A gateway that routes to several upstream providers may guarantee conformance
> for some of them and treat the schema as a hint for others. A finding through
> one does not identify which engine produced it, since support is per endpoint
> rather than per model.
