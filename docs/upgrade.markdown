Upgrading
=========

```sh
jsonschema upgrade <schema.json|.yaml> [--to/-t draft4|draft6|draft7|2019-09|2020-12]
  [--http/-h] [--verbose/-v] [--debug/-g] [--json/-j]
  [--header/-H "<name>: <value>"]
  [--resolve/-r <schemas-or-directories> ...]
  [--default-dialect/-d <uri>]
```

JSON Schema dialects are not always backwards compatible. The `upgrade` command
rewrites a schema to conform to a newer dialect, taking every subtletly across
specifications into account, including re-writing references that point at
locations whose path has changed. By default, schemas are upgraded to the
latest supported dialect, and the result is printed to standard output.

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
> We don't support upgrading schemas with custom meta-schemas, as both the
> schema and its meta-schema would need to be upgraded together.

> [!NOTE]
> The `--to/-t` option means "upgrade to at least this dialect". If your
> schema is already at or beyond the target dialect, the command leaves
> the schema unchanged. For example, asking the CLI to upgrade a 2020-12
> schema to Draft 7 will do nothing.

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

### Upgrade a YAML JSON Schema (output is JSON)

```sh
jsonschema upgrade path/to/schema.yaml
```
