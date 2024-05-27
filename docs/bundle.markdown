Bundling
========

```sh
jsonschema bundle <path/to/schema.json>
```

A schema may contain references to remote schemas located in other files or
even shared over the Internet. JSON Schema supports a standardized process,
referred to as
[bundling](https://json-schema.org/blog/posts/bundling-json-schema-compound-documents),
to resolve remote references in advance and inline them into the given schema.
The JSON Schema CLI supports this functionality through the `bundle` command.

Examples
--------

<!-- TODO: Actually exemplify real remote bundling -->

### Bundle a JSON Schema

```sh
jsonschema bundle path/to/my/schema.json
```
