Bundling
========

```sh
jsonschema bundle <schema.json>
  [--http/-h] [--verbose/-v] [--resolve/-r <schemas-or-directories> ...]
```

A schema may contain references to remote schemas outside the scope of the
given schema. These remote schemas may live in other files, or may be server by
others over the Internet. JSON Schema supports a standardized process, referred
to as
[bundling](https://json-schema.org/blog/posts/bundling-json-schema-compound-documents),
to resolve remote references in advance and inline them into the given schema
for local consumption or further distribution.  The JSON Schema CLI supports
this functionality through the `bundle` command.

Examples
--------

For example, consider the following schema that references a
`https://example.com/string` remote schema:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://example.com/string"
}
```

The referenced schema is defined in a `string.json` file that looks like this:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/string",
  "type": "string"
}
```

We can bundle the former schema by registering the latter schema into the
resolution context using the `--resolve / -r` option as follows:

```sh
jsonschema bundle schema.json --resolve string.json
```

The resulting schema, which will be printed to standard output, would look like
this:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://example.com/string",
  "$defs": {
    "https://example.com/string": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/string",
      "type": "string"
    }
  }
}
```

### Bundle a JSON Schema importing a single local schema

```sh
jsonschema bundle path/to/my/schema.json --resolve path/to/external.json
```

### Bundle a JSON Schema importing multiple local schemas

```sh
jsonschema bundle path/to/my/schema.json \
  --resolve path/to/one.json \
  --resolve path/to/two.json \
  --resolve path/to/three.json
```

### Bundle a JSON Schema importing a directory of `.schema.json` schemas

```sh
jsonschema bundle path/to/my/schema.json \
  --resolve path/to/schemas --extension schema.json
```

### Bundle a JSON Schema while enabling HTTP resolution

```sh
jsonschema bundle path/to/my/schema.json --http
```
