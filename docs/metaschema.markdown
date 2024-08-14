Metaschema
==========

> [!WARNING]
> JSON Schema Draft 3 and older are not supported at this point in time.

```sh
jsonschema metaschema [schemas-or-directories...]
  [--verbose/-v] [--http/-h] [--extension/-e <extension>]
  [--ignore/-i <schemas-or-directories>]
```

Ensure that a schema or a set of schemas are considered valid with regards to
their metaschemas.

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
