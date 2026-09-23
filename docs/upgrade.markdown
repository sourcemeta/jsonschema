Upgrading
=========

```sh
jsonschema upgrade <schema.json|.yaml> [--to/-t draft4|draft6|draft7|2019-09|2020-12]
  [--http/-h] [--verbose/-v] [--debug/-g] [--json/-j]
  [--header/-H "<name>: <value>"]
  [--resolve/-r <schemas-or-directories> ...]
  [--default-dialect/-d <uri>] [--configuration/-C <path>]
  [--color auto|always|never]
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
> This command refuses to upgrade meta-schemas, and refuses to upgrade schemas
> that declare one of their own. See [Meta-schemas cannot be
> upgraded](#meta-schemas-cannot-be-upgraded) for why, and for what to do
> instead.

> [!NOTE]
> The `--to/-t` option means "upgrade to at least this dialect". If your
> schema is already at or beyond the target dialect, the command leaves
> the schema unchanged. For example, asking the CLI to upgrade a 2020-12
> schema to Draft 7 will do nothing.

Meta-schemas cannot be upgraded
-------------------------------

A meta-schema describes a dialect, and it describes it by naming that dialect's
keywords as ordinary data. Upgrading a schema renames its keywords, but nothing
renames the data in a meta-schema that was talking about them. Take this Draft 7
meta-schema, which insists that subschemas live under `definitions`:

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://example.com/meta",
  "type": "object",
  "required": [ "definitions" ]
}
```

Upgrading a schema that declares this dialect to 2020-12 renames its
`definitions` keyword to `$defs`, while the `"definitions"` string inside
`required` stays as it is, because it is a string and not a keyword. The
upgraded schema no longer matches the dialect it declares, and neither document
is wrong on its own. Nothing in either file says whether `"definitions"` there
means the keyword or a property of that name, so the command refuses rather than
guess:

- Upgrading a document that describes itself, which is to say one whose `$id`
  names the dialect its `$schema` names, fails with `Cannot upgrade a schema
  that is itself a meta-schema`. The same applies to a document that carries,
  as an embedded resource, the meta-schema that something in it declares.
- Upgrading a schema that declares a meta-schema of its own fails with `Cannot
  upgrade a schema that uses a custom meta-schema`, whether that meta-schema is
  reachable through the resolver or bundled into the same document.

In both cases, upgrade the meta-schema and the schemas that declare it by hand,
deciding for yourself what each name in the meta-schema was meant to mean.

> [!NOTE]
> A document that does **not** describe itself is indistinguishable from an
> ordinary schema, and this command upgrades it like one. A Draft 7 meta-schema
> identified as `https://example.com/meta` that declares
> `http://json-schema.org/draft-07/schema#` as its `$schema` is a well-formed
> Draft 7 schema, and nothing in it says it is a meta-schema. Bundle such a
> meta-schema together with a schema that declares it if you want the command
> to recognise it.

> [!NOTE]
> From JSON Schema 2019-09 onwards, meta-schemas are required to declare a
> `$vocabulary` keyword. Because meta-schemas are no longer upgraded, this
> command no longer synthesizes that keyword, and the `--meta/-m` flag that
> used to request it is gone.

Dialects that cannot be upgraded
--------------------------------

Only the dialects that `--to/-t` accepts, plus Draft 3, can be upgraded from.
Anything else fails with `Upgrading schemas from this dialect is not supported
yet`, including Draft 0 through Draft 2 and every hyper-schema dialect. This
applies per resource, so a document whose root sits on a supported dialect still
fails if any resource embedded in it does not.

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
