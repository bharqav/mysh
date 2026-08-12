#!/usr/bin/env bash
set -euo pipefail

status=0

if command -v clang-format >/dev/null 2>&1; then
  echo "running clang-format check"
  cpp_files=()
  while IFS= read -r file; do
    cpp_files+=("$file")
  done < <(find . -type f \( -name '*.cpp' -o -name '*.hpp' \) -not -path './.git/*')
  if [[ ${#cpp_files[@]} -gt 0 ]]; then
    clang-format --dry-run --Werror "${cpp_files[@]}" || status=1
  fi
else
  echo "clang-format not found, skipping"
fi

if command -v cppcheck >/dev/null 2>&1; then
  echo "running cppcheck"
  cppcheck --enable=warning,performance,portability --error-exitcode=1 --quiet \
    --suppress=missingIncludeSystem . || status=1
else
  echo "cppcheck not found, skipping"
fi

if command -v clang-tidy >/dev/null 2>&1; then
  echo "running clang-tidy"
  cpp_files=()
  while IFS= read -r file; do
    cpp_files+=("$file")
  done < <(find . -type f -name '*.cpp' -not -path './.git/*' -not -path './tests/*')
  if [[ ${#cpp_files[@]} -gt 0 ]]; then
    clang-tidy "${cpp_files[@]}" -- -std=c++17 || status=1
  fi
else
  echo "clang-tidy not found, skipping"
fi

if [[ $status -ne 0 ]]; then
  echo "lint checks failed"
  exit 1
fi

echo "lint checks passed"
