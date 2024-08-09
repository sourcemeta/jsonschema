Validating
==========

> [!WARNING]
> Only JSON Schema Draft 4, Draft 6, Draft 7, and 2019-09 are supported at this
> point in time. We have plans to support *every* dialect of JSON Schema from
> Draft 0 to Draft 2020-12 soon.

```sh
jsonschema validate <schema.json> <instance.json|.jsonl...> [--http/-h]
  [--verbose/-v] [--resolve/-r <schemas-or-directories> ...] [--benchmark/-b]
```

The most popular use case of JSON Schema is to validate JSON documents. The
JSON Schema CLI offers a `validate` command to evaluate one or many JSON
instances or JSONL datasets against a JSON Schema, presenting human-friendly
information on unsuccessful validation.

**If you want to validate that a schema adheres to its metaschema, use the
[`metaschema`](./metaschema.markdown) command instead.**

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
