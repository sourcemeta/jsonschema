Validating
==========

> [!WARNING]
> JSON Schema Draft 3 and older are not supported at this point in time.

```sh
jsonschema validate <schema.json|.yaml> <instance.json|.jsonl|.yaml...>
  [--http/-h] [--verbose/-v] [--resolve/-r <schemas-or-directories> ...]
  [--benchmark/-b] [--extension/-e <extension>]
  [--ignore/-i <schemas-or-directories>] [--trace/-t] [--fast/-f]
  [--default-dialect/-d <uri>] [--template/-m <template.json>] [--json/-j]
```

The most popular use case of JSON Schema is to validate JSON documents. The
JSON Schema CLI offers a `validate` command to evaluate one or many JSON
instances or JSONL datasets against a JSON Schema, presenting human-friendly
information on unsuccessful validation.

The `--json`/`-j` option outputs the evaluation result using the JSON Schema
[`Flag`](https://json-schema.org/draft/2020-12/json-schema-core#section-12.4.1) or
[`Basic`](https://json-schema.org/draft/2020-12/json-schema-core#section-12.4.2)
standard format depending on whether the `--fast`/`-f` option is set.

**If you want to validate that a schema adheres to its metaschema, use the
[`metaschema`](./metaschema.markdown) command instead.**

To help scripts distinguish validation errors, these are reported using exit
code 2.

> [!NOTE]
> Annotations are only printed when passing the `--verbose`/`-v` or the
> `--trace`/`-t` options. However, annotation collection will be skipped if the
> `--fast`/`-f` option is passed.

> [!WARNING]
> By default, schemas are validated in exhaustive mode, which results in better
> error messages, at the expense of speed. The `--fast`/`-f` option makes the
> schema compiler optimise for speed, at the expense of error messages.

To speed-up the compilation process, you may pre-compile a schema using the
[`compile`](./compile.markdown) command and pass the result using the
`--template`/`-m` option. However, you still need to pass the original schema
for error reporting purposes. Make sure they match and that the compilation and
evaluation were done with the same version of this tool or you might get
non-sense results.

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

### Validate a JSON instance enabling HTTP resolution

```sh
jsonschema validate path/to/my/schema.json path/to/my/instance.json --http
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
