#!/bin/bash

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPLETION_SCRIPT="$(dirname "$SCRIPT_DIR")/completion/jsonschema.bash"

if ! [ -f "$COMPLETION_SCRIPT" ]
then
  echo "FAIL: Completion script not found at $COMPLETION_SCRIPT" 1>&2
  exit 1
fi

# shellcheck source=/dev/null
source "$COMPLETION_SCRIPT"

test_completion() {
  local input="$1"
  local expected="$2"
  local description="$3"

  read -r -a COMP_WORDS <<< "$input"

  if [[ "$input" == *" " ]]; then
    COMP_WORDS+=("")
  fi

  COMP_CWORD=$((${#COMP_WORDS[@]} - 1))
  COMP_LINE="$input"
  COMP_POINT=${#COMP_LINE}
  COMPREPLY=()

  _jsonschema

  local found=0
  if [ "${#COMPREPLY[@]}" -gt 0 ]; then
    for completion in "${COMPREPLY[@]}"
    do
      if [ "$completion" = "$expected" ]
      then
        found=1
        break
      fi
    done
  fi

  if [ $found -eq 0 ]
  then
    echo "FAIL: $description" 1>&2
    echo "  Input: $input" 1>&2
    echo "  Expected: $expected" 1>&2
    if [ "${#COMPREPLY[@]}" -gt 0 ]; then
      echo "  Got: ${COMPREPLY[*]}" 1>&2
    else
      echo "  Got: (no completions)" 1>&2
    fi
    return 1
  fi
}

test_completion "jsonschema " "validate" "Command completion includes validate"
test_completion "jsonschema " "metaschema" "Command completion includes metaschema"
test_completion "jsonschema " "compile" "Command completion includes compile"
test_completion "jsonschema " "test" "Command completion includes test"
test_completion "jsonschema " "fmt" "Command completion includes fmt"
test_completion "jsonschema " "lint" "Command completion includes lint"
test_completion "jsonschema " "bundle" "Command completion includes bundle"
test_completion "jsonschema " "inspect" "Command completion includes inspect"
test_completion "jsonschema " "canonicalize" "Command completion includes canonicalize"
test_completion "jsonschema " "encode" "Command completion includes encode"
test_completion "jsonschema " "decode" "Command completion includes decode"
test_completion "jsonschema " "codegen" "Command completion includes codegen"
test_completion "jsonschema " "version" "Command completion includes version"
test_completion "jsonschema " "help" "Command completion includes help"

test_completion "jsonschema validate --" "--verbose" "Validate includes global option --verbose"
test_completion "jsonschema validate --" "--benchmark" "Validate includes --benchmark"
test_completion "jsonschema validate --" "--trace" "Validate includes --trace"
test_completion "jsonschema validate --" "--fast" "Validate includes --fast"

test_completion "jsonschema lint --" "--fix" "Lint includes --fix"
test_completion "jsonschema lint --" "--list" "Lint includes --list"

test_completion "jsonschema bundle --" "--without-id" "Bundle includes --without-id"

test_completion "jsonschema compile --" "--minify" "Compile includes --minify"

test_completion "jsonschema fmt --" "--check" "Fmt includes --check"
test_completion "jsonschema fmt --" "--keep-ordering" "Fmt includes --keep-ordering"

test_completion "jsonschema canonicalize --" "--http" "Canonicalize includes --http"
test_completion "jsonschema canonicalize --" "--verbose" "Canonicalize includes global option --verbose"

test_completion "jsonschema codegen --" "--name" "Codegen includes --name"
test_completion "jsonschema codegen --" "--target" "Codegen includes --target"
test_completion "jsonschema codegen --" "--verbose" "Codegen includes global option --verbose"

echo "PASS" 1>&2
