Testing
=======

```sh
jsonschema test [schemas-or-directories...]
```

Schemas are code. As such, you should run an automated unit testing suite
against them. The JSON Schema CLI provides a schema-oriented test runner
inspired by the [official JSON Schema test
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
