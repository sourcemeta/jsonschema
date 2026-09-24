Upgrading
=========

```sh
jsonschema upgrade <schema.json|.yaml> [--to/-t draft4|draft6|draft7|2019-09|2020-12]
  [--http/-h] [--verbose/-v] [--debug/-g] [--json/-j]
  [--header/-H "<name>: <value>"]
  [--resolve/-r <schemas-or-directories> ...]
  [--default-dialect/-d <uri>] [--configuration/-C <path>]
  [--indentation/-n <spaces>] [--color auto|always|never]
```

> [!NOTE]
> See [Resolving External References](./guides/resolution.markdown) for every way of
> making referenced schemas available, including how to handle a reference whose
> URI differs from the identifier the target schema declares.

JSON Schema dialects are not always backwards compatible. The `upgrade` command
rewrites a schema to conform to a newer dialect, taking every subtletly across
specifications into account, including re-writing references that point at
locations whose path has changed. By default, schemas are upgraded to the
latest supported dialect, and the result is printed to standard output.

The result is printed in the format the input was written in: a YAML schema
upgrades into YAML and a JSON schema into JSON. Each level of nesting keeps the
width the input was written with, which `--indentation/-n` overrides.

For example, consider the following Draft 3 schema:

```json
{
  "$schema": "http://json-schema.org/draft-03/schema#",
  "id": "https://example.com",
  "type": "object",
  "properties": {
    "name": { "type": "string", "minLength": 1, "required": true },
    "born": { "$ref": "#/definitions/year" }
  },
  "definitions": {
    "year": { "type": "integer", "minimum": 1900, "divisibleBy": 1 }
  }
}
```

You can upgrade it to JSON Schema 2020-12 as follows:

```sh
jsonschema upgrade path/to/schema.json --to 2020-12
```

The result will be something like this:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "type": "object",
  "required": [ "name" ],
  "properties": {
    "name": {
      "type": "string",
      "minLength": 1
    },
    "born": {
      "$ref": "#/$defs/year"
    }
  },
  "$defs": {
    "year": {
      "type": "integer",
      "minimum": 1900,
      "multipleOf": 1
    }
  }
}
```

> [!WARNING]
> We don't support upgrading meta-schemas, nor schemas that declare a custom
> meta-schema. A meta-schema describes a dialect by naming that dialect's
> keywords as ordinary data. Upgrading renames the keywords, but nothing
> renames the data that was talking about them, so a meta-schema requiring
> `definitions` still requires it after its schemas moved to `$defs`. Upgrade
> both by hand instead.

> [!NOTE]
> The `--to/-t` option means "upgrade to at least this dialect". If your
> schema is already at or beyond the target dialect, the command leaves
> the schema unchanged. For example, asking the CLI to upgrade a 2020-12
> schema to Draft 7 will do nothing.

> [!NOTE]
> A meta-schema is only recognised as one if the document describes itself, or
> if it travels in the same document as a schema that declares it. Otherwise it
> is indistinguishable from an ordinary schema, and gets upgraded like one.

> [!NOTE]
> Every resource in the document must sit on Draft 3 or newer, and not on a
> hyper-schema dialect, as we don't support upgrading from those yet.

Examples
--------

### Upgrade a JSON Schema to the latest dialect

```sh
jsonschema upgrade path/to/schema.json
```

### Upgrade a JSON Schema to a specific dialect

```sh
jsonschema upgrade path/to/schema.json --to draft7
```

### Upgrade a JSON Schema piped from standard input

```sh
cat path/to/schema.json | jsonschema upgrade -
```

### Upgrade a JSON Schema that does not declare `$schema`

```sh
jsonschema upgrade path/to/schema.json \
  --default-dialect http://json-schema.org/draft-04/schema#
```

### Upgrade a JSON Schema that references external schemas

```sh
jsonschema upgrade path/to/schema.json \
  --resolve path/to/imported.json
```

### Upgrade a YAML JSON Schema

```sh
jsonschema upgrade path/to/schema.yaml
```
