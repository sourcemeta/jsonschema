Installing Dependencies (CI)
============================

```sh
jsonschema ci [--verbose/-v] [--debug/-g] [--json/-j]
```

The `ci` command strictly installs dependencies from the lock file for CI/CD
environments. It verifies that every dependency in
[`jsonschema.json`](./configuration.markdown) matches the lock file and that
every installed file has the expected hash. Unlike
[`install`](./install.markdown), this command never modifies the lock file and
fails on any mismatch.

This is analogous to `npm ci` in the Node.js ecosystem: it provides fast,
reproducible installs by trusting the lock file as the source of truth.

> [!TIP]
> Commit `jsonschema.lock.json` to version control so that `jsonschema ci` can
> reproduce the exact same dependency state across environments.

Examples
--------

### Verify and install dependencies from the lock file

```sh
jsonschema ci
```

### Verify and install dependencies with verbose output

```sh
jsonschema ci --verbose
```

### Verify and install dependencies with JSON output

```sh
jsonschema ci --json
```
