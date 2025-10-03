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
such as the [Sourcemeta Registry](https://registry.sourcemeta.com).

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
| `path` | String | Relative path to the directory containing your schemas (**not used in this CLI yet**) | Directory containing `jsonschema.json` |
| `defaultDialect` | String | The default JSON Schema dialect to use when a schema doesn't specify `$schema` | - |
| `resolve` | Object | A mapping of URIs to local file paths or other URIs for schema resolution remapping | `{}` |

Lookup Algorithm
----------------

The JSON Schema CLI automatically discovers `jsonschema.json` configuration files using an ancestor lookup algorithm, similar to how NPM locates `package.json` files. When processing a schema, the CLI first looks for a `jsonschema.json` file in the same directory as the schema. If not found, it searches the parent directory, then the parent's parent directory, and so on, until a configuration file is found or the filesystem root is reached.
