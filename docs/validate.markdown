Validating
==========

> [!WARNING]
> JSON Schema Draft 2 and older are not supported at this point in time.

```sh
jsonschema validate <schema.json|.yaml> <instance.json|.jsonl|.jsonl.gz|.yaml|directory...>
  [--http/-h] [--verbose/-v] [--debug/-g]
  [--header/-H "<name>: <value>"]
  [--resolve/-r <schemas-or-directories> ...]
  [--benchmark/-b] [--loop <iterations>] [--extension/-e <extension>]
  [--ignore/-i <schemas-or-directories>] [--trace/-t] [--fast/-f]
  [--template/-m <template.json>] [--json/-j] [--entrypoint/-p <pointer|uri>]
  [--continue/-c] [--format-assertion/-F]
```

The most popular use case of JSON Schema is to validate JSON documents. The
JSON Schema CLI offers a `validate` command to evaluate one or many JSON
instances, directories of instances, or JSONL datasets against a JSON Schema,
presenting human-friendly information on unsuccessful validation.

The `--json`/`-j` option outputs the evaluation result using the JSON Schema
[`Flag`](https://json-schema.org/draft/2020-12/json-schema-core#section-12.4.1) or
[`Basic`](https://json-schema.org/draft/2020-12/json-schema-core#section-12.4.2)
standard format depending on whether the `--fast`/`-f` option is set.

**If you want to validate that a schema adheres to its metaschema, use the
[`metaschema`](./metaschema.markdown) command instead.**

To help scripts distinguish validation errors, these are reported using exit
code 2.

> [!NOTE]
> When validating JSONL datasets, the command stops at the first entry that
> fails validation. Pass `--continue`/`-c` to report all failing entries
> instead.

> [!TIP]
> GZIP-compressed JSONL datasets (`.jsonl.gz`) are transparently decompressed
> during validation. No additional options are needed.

> [!NOTE]
> Annotations are only printed when passing the `--verbose`/`-v` or the
> `--trace`/`-t` options. However, annotation collection will be skipped if the
> `--fast`/`-f` option is passed.

> [!WARNING]
> By default, schemas are validated in exhaustive mode, which results in better
> error messages, at the expense of speed. The `--fast`/`-f` option makes the
> schema compiler optimise for speed, at the expense of error messages.

> [!TIP]
> The `format` keyword is treated as an annotation by default across all
> dialects (except 2020-12 with the [Format
> Assertion](https://www.learnjsonschema.com/2020-12/format-assertion/)
> vocabulary enabled). To force `format` to behave as an assertion, you have
> two options. Pass `--format-assertion`/`-F` to force every `format` in the
> schema to assert. Alternatively, set the custom `x-format-assertion` keyword
> to `true` as a sibling to a specific `format` to force assertion at that
> location only. Both mechanisms work across all supported dialects.

For example, the following Draft 4 schema asserts that the instance is a valid
email address rather than just annotating it:

```json
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string",
  "format": "email",
  "x-format-assertion": true
}
```

To speed-up the compilation process, you may pre-compile a schema using the
[`compile`](./compile.markdown) command and pass the result using the
`--template`/`-m` option. However, you still need to pass the original schema
for error reporting purposes. Make sure they match and that the compilation and
evaluation were done with the same version of this tool or you might get
non-sense results.

> [!TIP]
> Templates produced by the [`compile`](./compile.markdown) command can
> also be evaluated from JavaScript using the [Blaze JavaScript
> port](https://github.com/sourcemeta/blaze/tree/main/ports/javascript), a
> pure-JavaScript evaluator for browsers and JavaScript runtimes like
> Node.js, letting you compile with this CLI and validate anywhere.

Examples
--------

For example, consider the following JSON Schema Draft 4 schema that asserts
that the JSON instance is a string:

```json
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
```

Also consider a JSON instance called `instance.json` that looks like this:

```json
12345
```

This instance is an integer, while the given schema expects a string.
Validating the instance against the schema using the JSON Schema CLI will
result in the following output:

```sh
$ jsonschema validate schema.json instance.json
error: The target document is expected to be of the given type
    at instance location ""
    at evaluate path "/type"
```

### Validate a JSON instance against a schema

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json
```

### Validate a JSON instance against a schema and print the result as JSON

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json --json
```

### Validate a JSON instance against a schema with a pre-compiled template

```sh
jsonschema compile path/to/my/schema.json > template.json
jsonschema validate path/to/my/schema.json path/to/my/instance.json \
  --template template.json
```

### Validate a JSON instance against a schema in fast mode

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json --fast
```

### Validate a JSON instance forcing every `format` to assert

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json --format-assertion
```

### Validate a multiple JSON instances against a schema

```sh
jsonschema validate path/to/my/schema.json \
  path/to/my/instance_1.json \
  path/to/my/instance_2.json \
  path/to/my/instance_3.json
```

### Validate a JSONL dataset against a schema

```sh
jsonschema validate path/to/my/schema.json path/to/my/dataset.jsonl
```

### Validate a GZIP-compressed JSONL dataset against a schema

```sh
jsonschema validate path/to/my/schema.json path/to/my/dataset.jsonl.gz
```

### Validate a JSONL dataset reporting all failures

```sh
jsonschema validate path/to/my/schema.json path/to/my/dataset.jsonl --continue
```

Note that even with `--continue`, only the first error of each failing entry is
reported. Continuing validation past the first error within a single entry is a
gray area not covered by the JSON Schema specification and potentially tricky to
implement correctly at the evaluation level.

### Validate a JSON instance enabling HTTP resolution

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json --http
```

### Validate a JSON instance with a custom HTTP header

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json \
  --http --header "Authorization: Bearer $REGISTRY_TOKEN"
```

### Validate a JSON instance importing a single local schema

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json \
  --resolve path/to/external.json
```

### Validate a JSON instance importing a directory of `.schema.json` schemas

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json \
  --resolve path/to/schemas --extension schema.json
```

### Validate a JSON instance against a schema printing timing information

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json --benchmark
```

### Validate a JSON instance against a schema with trace information

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json --trace
```

### Validate a directory of instances against a schema

```sh
jsonschema validate path/to/my/schema.json path/to/instances/
```

### Validate a directory of instances with a specific extension

```sh
jsonschema validate path/to/my/schema.json path/to/instances/ --extension .data.json
```

### Validate a directory of instances while ignoring certain paths

```sh
jsonschema validate path/to/my/schema.json path/to/instances/ \
  --ignore path/to/instances/drafts
```

### Validate a JSON instance against a specific subschema

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json \
  --entrypoint '/$defs/MyType'
```
