#!/usr/bin/env python3
"""Interpreter for the CLI test DSL. Standard library only."""

import argparse
import difflib
import gzip
import os
import pathlib
import re
import shlex
import shutil
import subprocess
import sys
import tempfile


class TestFailure(Exception):
    def __init__(self, line_number, message):
        super().__init__(message)
        self.line_number = line_number
        self.message = message


RESERVED = {"CWD", "CWD_URI"}


class Interpreter:
    def __init__(self, binary, sandbox, environment):
        self.binary = binary
        self.sandbox = sandbox
        self.environment = dict(environment)
        self.environment["CWD"] = sandbox
        # The same sandbox as a file:// URI. Concatenating "file://" with the
        # path only works where the path starts with a separator, so it is
        # correct on UNIX and wrong on Windows, where "file://C:/x" parses the
        # drive letter as the URI authority
        self.environment["CWD_URI"] = pathlib.Path(sandbox).as_uri()

    # -- variables ---------------------------------------------------------

    def expand(self, token, line_number):
        def replace(match):
            if match.group(0) == "$$":
                return "$"
            name = match.group(1) or match.group(2)
            if name not in self.environment:
                raise TestFailure(line_number, f"undefined variable: ${name}")
            return self.environment[name]

        return re.sub(
            r"\$\$|\$\{([A-Za-z_][A-Za-z0-9_]*)\}|\$([A-Za-z_][A-Za-z0-9_]*)",
            replace, token)

    def resolve(self, path, line_number):
        expanded = self.expand(path, line_number)
        if os.path.isabs(expanded):
            return expanded
        return os.path.join(self.sandbox, expanded)

    # -- filters -----------------------------------------------------------

    def read(self, path, line_number):
        try:
            with open(path, "r", encoding="utf-8", newline="") as handle:
                return handle.read()
        except FileNotFoundError:
            raise TestFailure(line_number, f"no such file: {path}")

    def write(self, path, content):
        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="") as handle:
            handle.write(content)

    # -- commands ----------------------------------------------------------

    def command_write(self, operands, body, line_number):
        if len(operands) != 3 or operands[1] != "UNTIL":
            raise TestFailure(line_number, "usage: WRITE <path> UNTIL <terminator>")
        self.write(self.resolve(operands[0], line_number), body)

    def command_env(self, operands, line_number):
        if len(operands) != 2:
            raise TestFailure(line_number, "usage: ENV <name> <value>")
        name, value = operands
        if name in RESERVED:
            raise TestFailure(line_number, f"{name} is reserved and cannot be set")
        self.environment[name] = self.expand(value, line_number)

    def command_run(self, operands, line_number):
        if len(operands) < 8:
            raise TestFailure(line_number, "RUN needs STDIN, IN, INTO and EXPECTING")
        trailer = operands[-8:]
        if trailer[0] != "STDIN" or trailer[2] != "IN" \
                or trailer[4] != "INTO" or trailer[6] != "EXPECTING":
            raise TestFailure(
                line_number,
                "usage: RUN [arguments...] STDIN <file> IN <dir> INTO <dest> EXPECTING <code>")

        arguments = [self.expand(token, line_number) for token in operands[:-8]]
        stdin_name = self.expand(trailer[1], line_number)
        directory = self.resolve(trailer[3], line_number)
        destination = self.resolve(trailer[5], line_number)
        try:
            expected_code = int(trailer[7])
        except ValueError:
            raise TestFailure(line_number, f"not an exit code: {trailer[7]}")

        if stdin_name == "/dev/null":
            stdin_bytes = b""
        else:
            with open(self.resolve(stdin_name, line_number), "rb") as handle:
                stdin_bytes = handle.read()

        child_environment = {
            key: value for key, value in self.environment.items() if key != "CWD"
        }
        child_environment["PATH"] = os.environ.get("PATH", "")
        if sys.platform == "win32":
            for essential in ("SYSTEMROOT", "COMSPEC", "TEMP"):
                if essential in os.environ:
                    child_environment[essential] = os.environ[essential]

        completed = subprocess.run(
            [self.binary] + arguments,
            cwd=directory,
            env=child_environment,
            input=stdin_bytes,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        observation = self.observe(completed.stdout, completed.stderr)
        self.write(destination, observation)

        if completed.returncode != expected_code:
            raise TestFailure(
                line_number,
                f"expected exit {expected_code}, got {completed.returncode}\n"
                + observation)

    def observe(self, stdout_bytes, stderr_bytes):
        lines = []
        for prefix, raw in (("1>", stdout_bytes), ("2>", stderr_bytes)):
            text = raw.decode("utf-8", errors="replace").replace("\r\n", "\n")
            if not text:
                continue
            if text.endswith("\n"):
                text = text[:-1]
            for line in text.split("\n"):
                lines.append(prefix if line == "" else f"{prefix} {line}")
        return "".join(line + "\n" for line in lines)

    def command_compare(self, operands, line_number):
        if len(operands) != 3 or operands[1] != "AGAINST":
            raise TestFailure(line_number, "usage: COMPARE <actual> AGAINST <expected>")
        actual_path = self.resolve(operands[0], line_number)
        expected_path = self.resolve(operands[2], line_number)
        actual = self.read(actual_path, line_number)
        expected = self.read(expected_path, line_number)
        if actual != expected:
            diff = difflib.unified_diff(
                expected.splitlines(keepends=True),
                actual.splitlines(keepends=True),
                fromfile=operands[2], tofile=operands[0])
            raise TestFailure(line_number, "".join(diff))

    def command_filter(self, keyword, operands, line_number):
        if keyword == "REPLACE":
            if len(operands) != 5 or operands[1] != "WITH" or operands[3] != "IN":
                raise TestFailure(
                    line_number, "usage: REPLACE <regex> WITH <replacement> IN <file>")
            pattern, replacement, name = operands[0], operands[2], operands[4]
        else:
            if len(operands) != 5 or operands[0] != "LINES" \
                    or operands[1] != "MATCHING" or operands[3] != "IN":
                raise TestFailure(
                    line_number, f"usage: {keyword} LINES MATCHING <regex> IN <file>")
            pattern, replacement, name = operands[2], None, operands[4]

        path = self.resolve(name, line_number)
        content = self.read(path, line_number)
        expression = re.compile(self.expand(pattern, line_number), re.MULTILINE)

        if keyword == "REPLACE":
            content = expression.sub(
                self.expand(replacement, line_number).replace("\\", "\\\\"), content)
        else:
            keep = keyword == "KEEP"
            content = "".join(
                line for line in content.splitlines(keepends=True)
                if bool(expression.search(line)) == keep)
        self.write(path, content)

    def command_tree(self, operands, line_number):
        if len(operands) != 3 or operands[1] != "INTO":
            raise TestFailure(line_number, "usage: TREE <directory> INTO <destination>")
        root = self.resolve(operands[0], line_number)
        entries = []
        for parent, directories, files in os.walk(root):
            for name in directories + files:
                absolute = os.path.join(parent, name)
                relative = os.path.relpath(absolute, root).replace(os.sep, "/")
                entries.append("./" + relative)
        entries.sort()
        self.write(self.resolve(operands[2], line_number),
                   "".join(entry + "\n" for entry in entries))

    def command_copy(self, operands, line_number):
        if len(operands) != 3 or operands[1] != "TO":
            raise TestFailure(line_number, "usage: COPY <source> TO <destination>")
        source = self.resolve(operands[0], line_number)
        destination = self.resolve(operands[2], line_number)
        os.makedirs(os.path.dirname(destination) or ".", exist_ok=True)
        shutil.copy2(source, destination)

    def command_compress(self, operands, line_number):
        if len(operands) != 4 or operands[0] != "GZIP" or operands[2] != "INTO":
            raise TestFailure(
                line_number, "usage: COMPRESS GZIP <source> INTO <destination>")
        source = self.resolve(operands[1], line_number)
        destination = self.resolve(operands[3], line_number)
        try:
            with open(source, "rb") as handle:
                payload = handle.read()
        except FileNotFoundError:
            raise TestFailure(line_number, f"no such file: {source}")
        os.makedirs(os.path.dirname(destination) or ".", exist_ok=True)
        # A fixed modification time keeps the archive byte for byte reproducible,
        # as the gzip header would otherwise carry the time of the run
        with gzip.GzipFile(destination, "wb", mtime=0) as handle:
            handle.write(payload)

    def command_remove(self, operands, line_number):
        if len(operands) != 1:
            raise TestFailure(line_number, "usage: REMOVE <path>")
        path = self.resolve(operands[0], line_number)
        if os.path.isdir(path):
            shutil.rmtree(path, ignore_errors=True)
        elif os.path.exists(path):
            os.remove(path)

    def command_make_directory(self, operands, line_number):
        if len(operands) != 2 or operands[0] != "DIRECTORY":
            raise TestFailure(line_number, "usage: MAKE DIRECTORY <path>")
        os.makedirs(self.resolve(operands[1], line_number), exist_ok=True)


def run_script(path, binary, environment):
    with open(path, "r", encoding="utf-8", newline="") as handle:
        lines = handle.read().split("\n")

    sandbox = os.path.realpath(tempfile.mkdtemp(prefix="clitest-")).replace(os.sep, "/")
    interpreter = Interpreter(binary, sandbox, environment)
    try:
        index = 0
        while index < len(lines):
            line = lines[index]
            number = index + 1
            index += 1
            stripped = line.strip()
            if not stripped or stripped.startswith("//"):
                continue

            operands = shlex.split(stripped, posix=True)
            keyword = operands[0]
            rest = operands[1:]

            if keyword == "WRITE":
                terminator = rest[2] if len(rest) == 3 else None
                body = []
                while index < len(lines) and lines[index] != terminator:
                    body.append(lines[index])
                    index += 1
                if index >= len(lines):
                    raise TestFailure(number, f"unterminated WRITE, expected {terminator}")
                index += 1
                interpreter.command_write(
                    rest, "".join(item + "\n" for item in body), number)
            elif keyword == "RUN":
                interpreter.command_run(rest, number)
            elif keyword == "COMPARE":
                interpreter.command_compare(rest, number)
            elif keyword == "ENV":
                interpreter.command_env(rest, number)
            elif keyword in ("REPLACE", "DROP", "KEEP"):
                interpreter.command_filter(keyword, rest, number)
            elif keyword == "TREE":
                interpreter.command_tree(rest, number)
            elif keyword == "COPY":
                interpreter.command_copy(rest, number)
            elif keyword == "COMPRESS":
                interpreter.command_compress(rest, number)
            elif keyword == "REMOVE":
                interpreter.command_remove(rest, number)
            elif keyword == "MAKE":
                interpreter.command_make_directory(rest, number)
            else:
                raise TestFailure(number, f"unknown command: {keyword}")
    except TestFailure as failure:
        print(f"{path}:{failure.line_number}: {failure.message}", file=sys.stderr)
        return 1
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("script")
    parser.add_argument("--binary", required=True)
    parser.add_argument("--environment", action="append", default=[])
    arguments = parser.parse_args()

    environment = {}
    for item in arguments.environment:
        name, _, value = item.partition("=")
        environment[name] = value

    return run_script(arguments.script, arguments.binary, environment)


if __name__ == "__main__":
    sys.exit(main())
