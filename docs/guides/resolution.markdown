Resolving External References
=============================

A schema may reference other schemas that do not live inside it, using the
`$ref` keyword and friends. To make sense of such a schema, the JSON Schema CLI
needs to turn every referenced URI into an actual schema. The set of
associations it consults to do so is what this CLI calls the *resolution
context*, and you can think of it as a key value store from URI to schema.

The JSON Schema specification asks implementations to support precisely this,
in [Section
9.1.2](https://json-schema.org/draft/2020-12/json-schema-core#name-loading-a-referenced-schema):

> Implementations SHOULD be able to associate arbitrary URIs with an arbitrary
> schema and/or automatically associate a schema's `"$id"`-given URI, depending
> on the trust that the validator has in the schema.

The same section settles what a URI may point at:

> A schema MAY (and likely will) have multiple URIs, but there is no way for a
> URI to identify more than one schema. When multiple schemas try to identify
> as the same URI, validators SHOULD raise an error condition.

In other words, one schema may be reachable under several URIs, but asking a
single URI to mean two different schemas is an error that this CLI reports.

This page describes every way of putting a schema into the resolution context.
It applies to every command that supports the `--resolve/-r` option.

Importing Local Schemas
-----------------------

The `--resolve/-r` option imports a schema file into the resolution context
under the identifier that the schema declares for itself. For example, given a
schema that references `https://example.com/string`:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://example.com/string"
}
```

And a `string.json` file that declares that identifier:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/string",
  "type": "string"
}
```

Then `--resolve string.json` tells the CLI where to find it:

```sh
jsonschema bundle schema.json --resolve string.json
```

Because the association comes from the schema's own identifier, the file name
does not matter, but the identifier does. Importing a file whose identifier is
not the one being referenced has no effect on that reference.

You may pass `--resolve/-r` as many times as you need, and you may point it at
a directory to import every schema inside it:

```sh
jsonschema bundle schema.json \
  --resolve one.json \
  --resolve two.json \
  --resolve path/to/schemas
```

> [!WARNING]
> The `--resolve/-r` option takes exactly one value. If you pass it an unquoted
> shell glob such as `--resolve path/to/schemas/*.json`, your shell expands it
> before the CLI sees it, and only the first match is imported. The remaining
> matches silently become inputs to the command itself. Depending on the
> command, they are then processed as if you had asked for them, or ignored
> outright, and in both cases the schemas you meant to import are missing.
> Repeat the option per file, or point it at a directory, as shown above.

Relative References and the Base URI
------------------------------------

A relative reference such as `"$ref": "./string.json"` is **not** a file path.
It is a relative URI, and it is resolved against the schema's *base URI*, in
the same way that a relative link on a web page is resolved against the address
of that page.

What sets the base URI is the single most useful thing to understand here:

- If the schema declares an `$id`, that is the base URI.
- If it does not, the base URI is wherever the schema was loaded from, which
  for a local file is its `file://` URI.

This is not a rule this CLI invented. [JSON Schema 2020-12, Section
9.1.1](https://json-schema.org/draft/2020-12/json-schema-core#name-initial-base-uri)
defers to [RFC 3986, Section
5.1](https://datatracker.ietf.org/doc/html/rfc3986#section-5.1):

> Informatively, the initial base URI of a schema is the URI at which it was
> found, whether that was a network location, a local filesystem, or any other
> situation identifiable by a URI of any known scheme.
>
> If a schema document defines no explicit base URI with `"$id"` (embedded in
> content), the base URI is that determined per RFC 3986 section 5.

RFC 3986 lists the sources of a base URI in priority order: [Base URI Embedded
in Content](https://datatracker.ietf.org/doc/html/rfc3986#section-5.1.1), which
for JSON Schema means `$id`, then the encapsulating entity, then the [Retrieval
URI](https://datatracker.ietf.org/doc/html/rfc3986#section-5.1.3):

> If no base URI is embedded and the representation is not encapsulated within
> some other entity, then, if a URI was used to retrieve the representation,
> that URI shall be considered the base URI.

### Without an `$id`, relative references behave like file paths

Given `schema.json` next to `string.json`:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "./string.json"
}
```

There is no `$id`, so the base URI falls through to the retrieval URI, which is
the file itself. `./string.json` therefore resolves to the file sitting next to
it, and no `--resolve/-r` is needed at all. You can confirm this with
[`jsonschema inspect`](../inspect.markdown):

```sh
jsonschema inspect schema.json
```

```
(RESOURCE) URI: file:///home/me/schemas/schema.json
    ...
    Base              : file:///home/me/schemas/schema.json
    ...
(REFERENCE) ORIGIN: /$ref
    ...
    Destination       : file:///home/me/schemas/string.json
```

This is why relative references between plain, unidentified schema files simply
work, and it is a perfectly good way to organise a project.

### With an `$id`, they do not

Add an identifier and nothing else:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/schema",
  "$ref": "./string.json"
}
```

The base URI is now the `$id`, so the very same `$ref` resolves somewhere else
entirely:

```
(RESOURCE) URI: https://example.com/schema
    ...
    Base              : https://example.com/schema
    ...
(REFERENCE) ORIGIN: /$ref
    ...
    Destination       : https://example.com/string.json
```

The reference no longer points at any file, and the command fails:

```
error: Could not resolve the reference to an external schema
  at identifier https://example.com/string.json
  at file path /home/me/schemas/schema.json
```

> [!WARNING]
> This is a common pitfall. Adding an `$id` to a schema silently changes what
> every relative reference inside it means. Importing the neighbouring file with
> `--resolve string.json` does **not** fix it either, because that file declares
> no `$id` of its own and so gets imported under its `file://` URI, which is not
> the URI being looked for.

There are two ways to resolve it, and which one is right depends on whether you
control the target schema:

1. Give the target schema the identifier that the reference resolves to, here
   `https://example.com/string.json`, and import it with `--resolve/-r`. This is
   the tidiest option when the schemas are yours.
2. Remap the URI to the file, as described in the next section. This is the
   option when you cannot change the target schema.

Whenever a reference does not go where you expected, `jsonschema inspect` will
tell you the base URI in play and the exact destination each reference resolves
to.

Remapping a URI to a Schema
---------------------------

Sometimes a reference does not match the identifier that the target schema
declares. A common case is a schema published under a versioned URI, where
consumers reference the unversioned one:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/string?20260101",
  "type": "string"
}
```

Here `https://example.com/string` and `https://example.com/string?20260101` are
different URIs, so `--resolve/-r` alone cannot connect them. It only ever
imports a schema under the identifier the schema itself declares.

For these cases, declare the association explicitly using the `resolve`
property of the [`jsonschema.json`](../configuration.markdown) configuration
file, which maps a URI to a local file path or to another URI:

```json
{
  "resolve": {
    "https://example.com/string": "./schemas/string.json"
  }
}
```

Paths are relative to the directory holding `jsonschema.json`. Lookups are not
transitive, so the value of a matching entry is the final target and is not
itself looked up in `resolve` again.

This works without passing `--resolve/-r` at all, and is the mechanism to reach
for whenever a reference and an identifier genuinely differ.

Resolving Over HTTP
-------------------

The `--http/-h` option lets the CLI fetch any unresolved HTTP or HTTPS
reference over the network:

```sh
jsonschema bundle schema.json --http
```

Schemas imported through `--resolve/-r` or through the `resolve` configuration
property take precedence, so this only applies to references that are not
already known. Use `--header/-H` to pass credentials to a private registry:

```sh
jsonschema bundle schema.json \
  --http --header "Authorization: Bearer $REGISTRY_TOKEN"
```

Vendoring Dependencies
----------------------

If you would rather fetch remote schemas once and commit them, declare them as
`dependencies` in [`jsonschema.json`](../configuration.markdown) and run
[`jsonschema install`](../install.markdown). Installed dependencies are added to
the resolution context automatically, with no `--resolve/-r` needed.
