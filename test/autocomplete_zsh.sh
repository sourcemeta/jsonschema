#!/bin/zsh
# shellcheck shell=bash disable=SC1071,SC2034,SC2154,SC2207,SC2206

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
COMPLETION_SCRIPT="$(dirname "$SCRIPT_DIR")/completion/jsonschema.zsh"

if ! [ -f "$COMPLETION_SCRIPT" ]
then
  echo "FAIL: Completion script not found at $COMPLETION_SCRIPT" 1>&2
  exit 1
fi

ln -s "$COMPLETION_SCRIPT" "$TMP/_jsonschema"

fpath=("$TMP" $fpath)

autoload -U +X compinit
compinit -u

autoload -U +X _jsonschema

if ! typeset -f _jsonschema > /dev/null
then
  echo "FAIL: _jsonschema function not defined after loading completion script" 1>&2
  exit 1
fi

if ! grep -q "_jsonschema_extensions" "$COMPLETION_SCRIPT"
then
  echo "FAIL: _jsonschema_extensions helper function not found in completion script" 1>&2
  exit 1
fi

if ! grep -q "_jsonschema_dialects" "$COMPLETION_SCRIPT"
then
  echo "FAIL: _jsonschema_dialects helper function not found in completion script" 1>&2
  exit 1
fi

echo "PASS" 1>&2
