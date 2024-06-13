Bundling
========

```sh
jsonschema bundle <schema.json>
  [--http/-h] [--verbose/-v] [--resolve/-r <schemas-or-directories> ...]
  [--extension/-e <extension>] [--ignore/-i <schemas-or-directories>]
  [--without-id/-w]
```

A schema may contain references to remote schemas outside the scope of the
given schema. These remote schemas may live in other files, or may be server by
others over the Internet. JSON Schema supports a standardized process, referred
to as
[bundling](https://json-schema.org/blog/posts/bundling-json-schema-compound-documents),
to resolve remote references in advance and inline them into the given schema
for local consumption or further distribution.  The JSON Schema CLI supports
this functionality through the `bundle` command.

> [!WARNING]
> A popular use case for JSON Schema is providing auto-completion for code
> editors. If you plan to use your bundled schema for this, keep in mind that
> at the time of this writing, Visual Studio Code ships with a
> [non-compliant](https://bowtie.report/#/implementations/ts-vscode-json-languageservice)
> implementation that [does not support the `$id` and `id`
> keywords](https://github.com/microsoft/vscode-json-languageservice/issues/224)
> and that will fail to handle most bundled schemas. As a workaround, we offer
> the `--without-id`/`-w` option to perform bundling without relying on
> identifiers to make your resulting schemas compatible with Visual Studio
> Code.

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

### Bundle a JSON Schema importing a directory of schemas while ignoring another

```sh
jsonschema bundle path/to/my/schema.json \
  --resolve path/to/schemas --ignore path/to/schemas/nested
```

### Bundle a JSON Schema while enabling HTTP resolution

```sh
jsonschema bundle path/to/my/schema.json --http
```

### Bundle a JSON Schema without identifiers and importing a single local schema

```sh
jsonschema bundle path/to/my/schema.json --resolve path/to/external.json --without-id
```
