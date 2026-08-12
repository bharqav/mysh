#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./mysh}"
if [[ "$BIN" != /* ]]; then
  BIN="$PWD/$BIN"
fi

if [[ ! -x "$BIN" ]]; then
  echo "error: binary '$BIN' not found or not executable"
  exit 1
fi

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
cd "$tmpdir"

cat > regression.mysh <<'EOF'
echo alpha
export X=42
echo $X
echo one > f.txt
echo two >> f.txt
cat < f.txt
false && echo never
false || echo or-path
true && (false || echo grouped-ok)
false | true
echo pstatus:$?
true | false
echo pstatus:$?
export X=100 | cat
echo pstatus:$?
echo src > a.cpp
echo src > b.cpp
echo *.cpp
sleep 1 &
jobs
fg 1
sleep 2 &
jobs
bg 2
help
exit 0
EOF

"$BIN" < regression.mysh > out.txt 2>&1

assert_contains() {
  local needle="$1"
  if ! grep -q "$needle" out.txt; then
    echo "assertion failed: expected output to contain '$needle'"
    echo "----- output -----"
    cat out.txt
    echo "------------------"
    exit 1
  fi
}

assert_not_contains() {
  local needle="$1"
  if grep -q "$needle" out.txt; then
    echo "assertion failed: output unexpectedly contained '$needle'"
    echo "----- output -----"
    cat out.txt
    echo "------------------"
    exit 1
  fi
}

assert_contains "alpha"
assert_contains "42"
assert_contains "one"
assert_contains "two"
assert_contains "or-path"
assert_contains "grouped-ok"
assert_contains "pstatus:0"
assert_contains "pstatus:1"
assert_contains "export: not supported in pipelines/subprocess context"
assert_contains "a.cpp"
assert_contains "b.cpp"
assert_contains "Running sleep 1"
assert_contains "Built-ins:"
assert_not_contains "never"

echo "regression test passed"
