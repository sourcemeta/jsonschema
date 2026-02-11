Installing Dependencies
=======================

```sh
jsonschema install [<uri> <path>]
  [--force/-f] [--verbose/-v] [--debug/-g] [--json/-j]
```

Many applications rely on consuming schemas authored and maintained by others.
These schemas are typically published over HTTP(S) through registries like
[schemas.sourcemeta.com](https://schemas.sourcemeta.com) (a free public
registry) or self-hosted solutions like [Sourcemeta
One](https://one.sourcemeta.com). While you could manually download these
schemas or write a custom fetching script, doing so quickly gets complicated:
schemas often reference other schemas that also need to be fetched, and the
results need to be
[bundled](https://json-schema.org/blog/posts/bundling-json-schema-compound-documents)
to inline those references for local consumption. The `install` command solves
this in a single step.

You can quickly add a dependency by passing a URI and a local file path
directly on the command line. For example:

```sh
jsonschema install https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response ./vendor/response.json
```

This adds the dependency to `jsonschema.json` (creating the file if it does not
exist) and immediately fetches it. The path is stored relative to
`jsonschema.json`, so you can run this command from any subdirectory of your
project.

Alternatively, you can declare dependencies manually. Create a
[`jsonschema.json`](./configuration.markdown) configuration file in your
project and declare your dependencies as a mapping of schema URIs to local file
paths. For example, to pull in the [JSON-RPC 2.0
Response](https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response)
schema from the public registry:

```json
{
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response": "./vendor/response.json"
  }
}
```

Then run `jsonschema install`. Each dependency is fetched, bundled, and written
to the specified path. A `jsonschema.lock.json` lock file is created alongside
`jsonschema.json` to record the hash of each installed dependency. On
subsequent runs, only dependencies that are missing, modified, or not yet
tracked in the lock file are fetched again.

> [!TIP]
> We recommend committing `jsonschema.lock.json` to version control (similar to
> `package-lock.json`) to enable reproducible installs across environments.

> [!WARNING]
> If a dependency uses a custom meta-schema that is itself an external
> dependency, make sure to list both the meta-schema and the schema that uses
> it in the `dependencies` map. The install command will resolve the
> meta-schema during bundling regardless of processing order. We are working
> on resolving this automatically in a future release.

Examples
--------

### Add a dependency

```sh
jsonschema install https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response ./vendor/response.json
```

### Add a dependency and force re-fetch all

```sh
jsonschema install --force https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response ./vendor/response.json
```

### Install all dependencies

```sh
jsonschema install
```

### Force re-fetch all dependencies

```sh
jsonschema install --force
```

### Install with verbose output

```sh
jsonschema install --verbose
```
