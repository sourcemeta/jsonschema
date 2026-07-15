The `jsonschema.json` Configuration File
========================================

> [!WARNING]
> This configuration file format is under active development and is considered
> experimental. The format may change in future releases. We appreciate your
> feedback to help us improve this feature.

The JSON Schema CLI supports a `jsonschema.json` configuration file to simplify
working with JSON Schema projects. Similar to how NPM uses `package.json` to
manage JavaScript projects, `jsonschema.json` provides a centralized way to
configure schema resolution, default dialects, and project metadata for your
JSON Schema workflows.

It is meant to be used not only by this CLI but by other JSON Schema tooling
such as [Sourcemeta One](https://one.sourcemeta.com).

Example
-------

The `jsonschema.json` file format looks like this:

```json
{
  "title": "My JSON Schema Project",
  "description": "A collection of schemas for my application",
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema",
  "resolve": {
    "https://example.com/schemas/user": "./schemas/user.json",
    "https://example.com/schemas/product": "./schemas/product.json"
  },
  "ignore": [
    "./drafts"
  ],
  "lint": {
    "rules": [
      "./rules/require_type.json",
      { "path": "./rules/require_id.json", "topLevel": true }
    ]
  }
}
```

Reference
---------

All properties in the `jsonschema.json` file are optional. The following table
describes the available configuration options:

| Property | Type | Description | Default |
|-------|------|-------------|---------|
| `title` | String | The title or name of your JSON Schema project | - |
| `description` | String | A brief description of your JSON Schema project | - |
| `email` | String | Contact email for the project maintainer | - |
| `github` | String | GitHub owner and repository | - |
| `website` | String | Project website URL | - |
| `baseUri` | String | The base URI for your schemas (**not used in this CLI yet**) | - |
| `path` | String | Relative path to the directory containing your schemas | Directory containing `jsonschema.json` |
| `defaultDialect` | String | The default JSON Schema dialect to use when a schema doesn't specify `$schema`. May be an absolute URI or a relative path, which is resolved to a canonical `file://` URI relative to the directory containing `jsonschema.json` (the `path` property does not shift this base). Relative resolution is always against the configuration file, never against a schema's own `$id`. The same resolution applies to the `--default-dialect` command-line flag, using the current working directory as the base | - |
| `extension` | String / String[] | The schema extension/s used by the project | `.json` / `.yaml` / `.yml` |
| `ignore` | String[] | Paths to ignore relative to `jsonschema.json`. Glob patterns are not currently supported | `[]` |
| `resolve` | Object | A mapping of URIs to local file paths or other URIs for schema resolution remapping. Lookups are non-transitive: the value of a matching entry is the final target and is not itself looked up in `resolve` again | `{}` |
| `dependencies` | Object | A mapping of URIs to relative file paths for external schema dependencies to install (see [`jsonschema install`](./install.markdown)) | `{}` |
| `lint` | Object | Lint configuration | `{}` |
| `lint.rules` | (String / Object)[] | Paths to custom lint rule schemas relative to `jsonschema.json` (see [lint](./lint.markdown)). An entry may also be an object with a required `path` string property and an optional `topLevel` boolean property (defaults to `false`) that makes the rule only run against the document root | `[]` |

Lookup Algorithm
----------------

The JSON Schema CLI automatically discovers `jsonschema.json` configuration files using an ancestor lookup algorithm, similar to how NPM locates `package.json` files. When processing a schema, the CLI first looks for a `jsonschema.json` file in the same directory as the schema. If not found, it searches the parent directory, then the parent's parent directory, and so on, until a configuration file is found or the filesystem root is reached.
