Metaschema
==========

> [!WARNING]
> JSON Schema Draft 3 and older are not supported at this point in time.

```sh
jsonschema metaschema [schemas-or-directories...]
  [--http/-h] [--verbose/-v] [--debug/-g] [--extension/-e <extension>]
  [--resolve/-r <schemas-or-directories> ...]
  [--ignore/-i <schemas-or-directories>] [--trace/-t]
  [--default-dialect/-d <uri>] [--json/-j]
```

Ensure that a schema or a set of schemas are considered valid with regards to
their metaschemas. The `--json`/`-j` option outputs the evaluation result using
the JSON Schema
[`Basic`](https://json-schema.org/draft/2020-12/json-schema-core#section-12.4.2)
standard format.

> [!WARNING]
> The point of this command is to help schema writers make sure their schemas
> are valid. As a consequence, this command prioritises useful error messages
> and exhaustive evaluation rather than validation speed. If you require fast
> validation, use the [`validate`](./validate.markdown) command with its
> `--fast`/`-f` option instead.

The `--resolve`/`-r` option is crucial to import custom meta-schemas into the
resolution context, otherwise the validator won't know where to look for them.

To help scripts distinguish validation errors, these are reported using exit
code 2.

Examples
--------

For example, consider this fictitious JSON Schema that follows the Draft 4
dialect but sets the `type` property to an invalid value:

```json
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
```

Running the `metaschema` command on it will surface an error:

```sh
$ jsonschema metaschema schema.json
error: The target document is expected to be one of the given values
    at instance location "/type"
    at evaluate path "/properties/type/anyOf/0/$ref/enum"
error: Mark the current position of the evaluation process for future jumps
    at instance location "/type"
    at evaluate path "/properties/type/anyOf/0/$ref"
error: The target document is expected to be of the given type
    at instance location "/type"
    at evaluate path "/properties/type/anyOf/1/type"
error: The target is expected to match at least one of the given assertions
    at instance location "/type"
    at evaluate path "/properties/type/anyOf"
error: The target is expected to match all of the given assertions
    at instance location ""
    at evaluate path "/properties"
```

### Validate the metaschema of JSON Schemas in-place

```sh
jsonschema metaschema path/to/my/schema_1.json path/to/my/schema_2.json
```

### Validate a JSON Schema with a custom meta-schema

```sh
jsonschema validate path/to/my/schema.json --resolve path/to/custom-meta-schema.json
```

### Validate the metaschema of every `.json` file in a given directory (recursively)

```sh
jsonschema metaschema path/to/schemas/
```

### Validate the metaschema of every `.json` file in the current directory (recursively)

```sh
jsonschema metaschema
```

### Validate the metaschema of every `.json` file in a given directory while ignoring another

```sh
jsonschema metaschema path/to/schemas/ --ignore path/to/schemas/nested
```

### Validate the metaschema of every `.schema.json` file in the current directory (recursively)

```sh
jsonschema metaschema --extension .schema.json
```

### Validate the metaschema of a JSON Schema with trace information

```sh
jsonschema metaschema path/to/my/schema.json --trace
```

### Validate the metaschema of a JSON Schema and print the result as JSON

```sh
jsonschema metaschema path/to/my/schema.json --json
```
