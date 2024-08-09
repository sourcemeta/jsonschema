Compile
=======

> [!WARNING]
> Only JSON Schema Draft 4, Draft 6, Draft 7, and 2019-09 are supported at this
> point in time. We have plans to support *every* dialect of JSON Schema from
> Draft 0 to Draft 2020-12 soon.

```sh
jsonschema compile <schema.json>
```

Evaluating a JSON Schema is a complex process with a non-trivial associated
performance overhead. For faster evaluation, we internally pre-process a given
JSON Schema into a compiled low-level form. This command allows you to obtain
the schema compilation result as a separate step.

**At least for now, this command is mainly for internal debugging purposes. As
an end-user, you are better served by the [`validate`](./validate.markdown)
command, which internally compiles schemas.**

Examples
--------

For example, consider the following simple schema:

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "properties": {
    "foo": { "type": "string" }
  }
}
```

The compilation process will result in something like this:

```
[
  {
    "category": "logical",
    "type": "and",
    "value": null,
    "absoluteKeywordLocation": "#/properties",
    "relativeSchemaLocation": "/properties",
    "relativeInstanceLocation": "",
    "target": {
      "category": "target",
      "type": "instance",
      "location": ""
    },
    "condition": [
      {
        "category": "assertion",
        "type": "type-strict",
        "value": {
          "category": "value",
          "type": "type",
          "value": "object"
        },
        "absoluteKeywordLocation": "#/properties",
        "relativeSchemaLocation": "",
        "relativeInstanceLocation": "",
        "target": {
          "category": "target",
          "type": "instance",
          "location": ""
        },
        "condition": []
      }
    ],
    "children": [
      {
        "category": "internal",
        "type": "container",
        "value": null,
        "absoluteKeywordLocation": "#/properties",
        "relativeSchemaLocation": "",
        "relativeInstanceLocation": "",
        "target": {
          "category": "target",
          "type": "instance",
          "location": ""
        },
        "condition": [
          {
            "category": "assertion",
            "type": "defines",
            "value": {
              "category": "value",
              "type": "string",
              "value": "foo"
            },
            "absoluteKeywordLocation": "#/properties",
            "relativeSchemaLocation": "",
            "relativeInstanceLocation": "",
            "target": {
              "category": "target",
              "type": "instance",
              "location": ""
            },
            "condition": []
          }
        ],
        "children": [
          {
            "category": "assertion",
            "type": "type-strict",
            "value": {
              "category": "value",
              "type": "type",
              "value": "string"
            },
            "absoluteKeywordLocation": "#/properties/foo/type",
            "relativeSchemaLocation": "/foo/type",
            "relativeInstanceLocation": "/foo",
            "target": {
              "category": "target",
              "type": "instance",
              "location": ""
            },
            "condition": []
          },
          {
            "category": "annotation",
            "type": "public",
            "value": {
              "category": "value",
              "type": "json",
              "value": "foo"
            },
            "absoluteKeywordLocation": "#/properties",
            "relativeSchemaLocation": "",
            "relativeInstanceLocation": "",
            "target": {
              "category": "target",
              "type": "instance",
              "location": ""
            },
            "condition": []
          }
        ]
      }
    ]
  }
]
```

### Compile a JSON Schema

```sh
jsonschema compile path/to/my/schema.json
```
