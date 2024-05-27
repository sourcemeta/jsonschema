Linting
=======

```sh
jsonschema lint <schemas-or-directories...> [--fix|-f]
```

JSON Schema is a surprisingly expressive schema language. Like with traditional
programming languages, writing efficient and maintainable schemas take
experience, and there are lots of ways of doing it wrong. To help with this,
the JSON Schema CLI provides a `lint` command that can check your schemas
against various common anti-patterns and automatically fix many of them.

Examples
--------

### Lint JSON Schemas in-place

```sh
jsonschema lint path/to/my/schema_1.json path/to/my/schema_2.json
```

### Lint every `.json` file in a given directory (recursively)

```sh
jsonschema lint path/to/schemas/
```

### Lint every `.json` file in the current directory (recursively)

```sh
jsonschema lint
```

### Fix lint warnings on a single schema

```sh
jsonschema lint path/to/my/schema.json --fix
```
