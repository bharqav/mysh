#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./mysh}"

if [[ ! -x "$BIN" ]]; then
  echo "error: binary '$BIN' not found or not executable"
  exit 1
fi

make check

if command -v gcovr >/dev/null 2>&1; then
  mkdir -p coverage
  gcovr --root . --xml-pretty --output coverage/coverage.xml
  gcovr --root . --html-details --output coverage/coverage.html
  gcovr --root . --print-summary
  echo "coverage reports generated in coverage/"
else
  echo "gcovr not installed, install it for structured coverage reports"
fi
