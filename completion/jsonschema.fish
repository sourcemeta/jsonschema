set -l commands validate metaschema compile test fmt lint bundle inspect encode decode version help

complete -c jsonschema -f

complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "validate" -d "Validate instances against a schema"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "metaschema" -d "Validate schemas against their metaschemas"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "compile" -d "Compile a schema into an optimised representation"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "test" -d "Run unit tests against a schema"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "fmt" -d "Format schemas in-place or check formatting"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "lint" -d "Lint schemas and optionally fix issues"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "bundle" -d "Inline remote references in a schema"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "inspect" -d "Display schema locations and references"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "encode" -d "Encode JSON using JSON BinPack"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "decode" -d "Decode JSON using JSON BinPack"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "version" -d "Print version information"
complete -c jsonschema -n "not __fish_seen_subcommand_from $commands" -a "help" -d "Print help information"

complete -c jsonschema -l verbose -s v -d "Enable verbose output"
complete -c jsonschema -l resolve -s r -r -F -d "Import schemas into resolution context"
complete -c jsonschema -l default-dialect -s d -x -d "Specify default dialect URI"
complete -c jsonschema -l json -s j -d "Prefer JSON output if supported"
complete -c jsonschema -l http -s h -d "Enable HTTP resolution"

complete -c jsonschema -n "__fish_seen_subcommand_from validate" -l benchmark -s b -d "Enable benchmarking mode"
complete -c jsonschema -n "__fish_seen_subcommand_from validate" -l loop -s l -x -d "Number of loop iterations"
complete -c jsonschema -n "__fish_seen_subcommand_from validate" -l extension -s e -x -a ".json .yaml .yml" -d "Specify file extension"
complete -c jsonschema -n "__fish_seen_subcommand_from validate" -l ignore -s i -r -F -d "Ignore schemas or directories"
complete -c jsonschema -n "__fish_seen_subcommand_from validate" -l trace -s t -d "Enable trace output"
complete -c jsonschema -n "__fish_seen_subcommand_from validate" -l fast -s f -d "Optimise for speed"
complete -c jsonschema -n "__fish_seen_subcommand_from validate" -l template -s m -r -F -d "Use pre-compiled schema template"
complete -c jsonschema -n "__fish_seen_subcommand_from validate" -F

complete -c jsonschema -n "__fish_seen_subcommand_from metaschema" -l extension -s e -x -a ".json .yaml .yml" -d "Specify file extension"
complete -c jsonschema -n "__fish_seen_subcommand_from metaschema" -l ignore -s i -r -F -d "Ignore schemas or directories"
complete -c jsonschema -n "__fish_seen_subcommand_from metaschema" -l trace -s t -d "Enable trace output"
complete -c jsonschema -n "__fish_seen_subcommand_from metaschema" -F

complete -c jsonschema -n "__fish_seen_subcommand_from compile" -l extension -s e -x -a ".json .yaml .yml" -d "Specify file extension"
complete -c jsonschema -n "__fish_seen_subcommand_from compile" -l ignore -s i -r -F -d "Ignore schemas or directories"
complete -c jsonschema -n "__fish_seen_subcommand_from compile" -l fast -s f -d "Optimise for speed"
complete -c jsonschema -n "__fish_seen_subcommand_from compile" -l minify -s m -d "Minify output"
complete -c jsonschema -n "__fish_seen_subcommand_from compile" -F

complete -c jsonschema -n "__fish_seen_subcommand_from test" -l extension -s e -x -a ".json .yaml .yml" -d "Specify file extension"
complete -c jsonschema -n "__fish_seen_subcommand_from test" -l ignore -s i -r -F -d "Ignore schemas or directories"
complete -c jsonschema -n "__fish_seen_subcommand_from test" -F

complete -c jsonschema -n "__fish_seen_subcommand_from fmt" -l check -s c -d "Check formatting without modifying"
complete -c jsonschema -n "__fish_seen_subcommand_from fmt" -l extension -s e -x -a ".json .yaml .yml" -d "Specify file extension"
complete -c jsonschema -n "__fish_seen_subcommand_from fmt" -l ignore -s i -r -F -d "Ignore schemas or directories"
complete -c jsonschema -n "__fish_seen_subcommand_from fmt" -l keep-ordering -s k -d "Keep original key ordering"
complete -c jsonschema -n "__fish_seen_subcommand_from fmt" -l indentation -s n -x -a "2 4 8" -d "Specify indentation spaces"
complete -c jsonschema -n "__fish_seen_subcommand_from fmt" -F

complete -c jsonschema -n "__fish_seen_subcommand_from lint" -l fix -s f -d "Fix issues automatically"
complete -c jsonschema -n "__fish_seen_subcommand_from lint" -l extension -s e -x -a ".json .yaml .yml" -d "Specify file extension"
complete -c jsonschema -n "__fish_seen_subcommand_from lint" -l ignore -s i -r -F -d "Ignore schemas or directories"
complete -c jsonschema -n "__fish_seen_subcommand_from lint" -l exclude -s x -x -d "Exclude specific rule"
complete -c jsonschema -n "__fish_seen_subcommand_from lint" -l only -s o -x -d "Only run specific rule"
complete -c jsonschema -n "__fish_seen_subcommand_from lint" -l list -s l -d "List all enabled rules"
complete -c jsonschema -n "__fish_seen_subcommand_from lint" -l strict -s s -d "Enable strict mode"
complete -c jsonschema -n "__fish_seen_subcommand_from lint" -l indentation -s n -x -a "2 4 8" -d "Specify indentation spaces"
complete -c jsonschema -n "__fish_seen_subcommand_from lint" -F

complete -c jsonschema -n "__fish_seen_subcommand_from bundle" -l extension -s e -x -a ".json .yaml .yml" -d "Specify file extension"
complete -c jsonschema -n "__fish_seen_subcommand_from bundle" -l ignore -s i -r -F -d "Ignore schemas or directories"
complete -c jsonschema -n "__fish_seen_subcommand_from bundle" -l without-id -s w -d "Bundle without ID"
complete -c jsonschema -n "__fish_seen_subcommand_from bundle" -F

complete -c jsonschema -n "__fish_seen_subcommand_from inspect" -F

complete -c jsonschema -n "__fish_seen_subcommand_from encode" -F

complete -c jsonschema -n "__fish_seen_subcommand_from decode" -F
