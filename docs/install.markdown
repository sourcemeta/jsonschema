Installing Dependencies
======================

```sh
jsonschema install
  [--force/-f] [--verbose/-v] [--debug/-g] [--json/-j]
```

The `install` command fetches and installs external schema dependencies
declared in the [`jsonschema.json`](./configuration.markdown) configuration
file. It produces a `jsonschema.lock.json` lock file that tracks the hashes of
installed dependencies, similar to how NPM uses `package-lock.json`.

The command discovers the nearest `jsonschema.json` by walking up from the
current working directory. Dependencies are declared as a mapping of URIs to
relative file paths:

```json
{
  "dependencies": {
    "https://example.com/schemas/user.json": "./vendor/user.json",
    "https://example.com/schemas/product.json": "./vendor/product.json"
  }
}
```

When you run `jsonschema install`, each dependency is fetched, bundled (to
inline any remote references), and written to the specified path. The lock file
`jsonschema.lock.json` is created or updated alongside `jsonschema.json` to
record the hash of each installed dependency.

On subsequent runs, the command only fetches dependencies that are missing,
modified, or not yet tracked in the lock file.

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
