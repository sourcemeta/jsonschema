Encode
======

```sh
jsonschema encode <document.json|.jsonl> <output.binpack>
  [--default-dialect, -d <uri>]
```

This command encodes a JSON document using [JSON
BinPack](https://jsonbinpack.sourcemeta.com) schema-less mode. **Note this
command is considered experimental and its output might not be decodable across
versions of this CLI**.

Examples
--------

For example, consider the following simple document:

```json
{
  "version": 2.0
}
```

The JSON BinPack schema-less encoding will result in something like this:

```
$ xxd output.binpack
00000000: 1308 7665 7273 696f 6e37 02              ..version7.
```

### Encode a JSON document

```sh
jsonschema encode path/to/my/document.json path/to/output.binpack
```

### Encode a JSONL dataset

```sh
jsonschema encode path/to/my/dataset.jsonl path/to/output.binpack
```
