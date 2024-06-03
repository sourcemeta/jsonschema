Testing
=======

> [!WARNING]
> Only JSON Schema Draft 4 is supported at this point in time. We have plans
> to support *every* dialect of JSON Schema from Draft 0 to Draft 2020-12 soon.

```sh
jsonschema test [schemas-or-directories...] [--http/-h] [--metaschema/-m] [--verbose/-v] [--resolve/-r <schema.json> ...]
```

Schemas are code. As such, you should run an automated unit testing suite
against them. Just like popular test frameworks like [Jest](https://jestjs.io),
[GoogleTest](https://google.github.io/googletest/), and
[PyTest](https://docs.pytest.org), the JSON Schema CLI provides a
schema-oriented test runner inspired by the [official JSON Schema test
suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite).

Examples
--------

To create a test definition, you must write JSON documents that look like this:

```json
{
  "description": "The Draft 4 metaschema",
  "schema": "http://json-schema.org/draft-04/schema#",
  "tests": [
    {
      "description": "The empty object is valid",
      "valid": true,
      "data": {}
    },
    {
      "description": "The `type` keyword must be a string",
      "valid": false,
      "data": {
        "type": 1
      }
    }
  ]
}
```

Assuming this file is saved as `test/draft4.json`, you can run it as follows:

```sh
jsonschema test test/draft4.json
```

### Run a single test definition

```sh
jsonschema test path/to/test.json
```

### Run every `.json` test definition in a given directory (recursively)

```sh
jsonschema test path/to/tests/
```

### Run every `.json` test definition in the current directory (recursively)

```sh
jsonschema test
```

### Run a single test definition validating the schemas against their metaschemas

```sh
jsonschema test path/to/test.json --metaschema
```

### Run a single test definition enabling HTTP resolution

```sh
jsonschema test path/to/test.json --http
```

### Run a single test definition importing a single local schema

```sh
jsonschema test path/to/test.json --resolve path/to/external.json
```
