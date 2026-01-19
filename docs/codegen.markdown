Codegen
=======

```sh
jsonschema codegen <schema.json|.yaml> --target/-t <target> [--name/-n <name>]
  [--json/-j] [--verbose/-v] [--resolve/-r <schemas-or-directories>]
  [--default-dialect/-d <uri>]
```

JSON Schema is a declarative language for defining the structure of JSON data.
The JSON Schema CLI offers a `codegen` command that generates type definitions
from a JSON Schema, allowing you to use the schema as a source of truth for
your application's data types.

**This command is experimental. We are actively working on this functionality
and things might break or change without notice. We are actively seeking
feedback to make this better. Please open an issue at
https://github.com/sourcemeta/jsonschema/issues with any problems you find.**

> [!WARNING]
> This command currently only supports JSON Schema 2020-12 and TypeScript as a
> target language.

The list of supported JSON Schema features is outlined at
https://github.com/sourcemeta/codegen.

Examples
--------

For example, consider the following schema that describes a person:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age": { "type": "integer" }
  },
  "required": [ "name" ]
}
```

The codegen command will generate the following TypeScript types:

```typescript
export type PersonName = string;

export type PersonAge = number;

export interface Person {
  "name": PersonName;
  "age"?: PersonAge;
  [key: string]: unknown | undefined;
}
```

### Generate TypeScript types from a JSON Schema

```sh
jsonschema codegen path/to/my/schema.json --target typescript
```

### Generate TypeScript types with a custom type name prefix

By default, generated types use `Schema` as the prefix. You can specify a
custom prefix using the `--name/-n` option:

```sh
jsonschema codegen path/to/my/schema.json --target typescript --name Person
```

### Output the generated code as JSON

```sh
jsonschema codegen path/to/my/schema.json --target typescript --json
```
