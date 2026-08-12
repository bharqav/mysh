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

cat > test_input.mysh <<'EOF'
echo hello
export FOO=bar
echo $FOO
echo "a \"quoted\" string"
VAR=10 echo $VAR
TEMP=ok
echo $TEMP
echo $(echo nested)
echo one ; echo two
(TEMP2=inner; echo subshell:$TEMP2)
echo outer:$TEMP2
echo hi > out.txt
cat < out.txt
echo add >> out.txt
cat out.txt
false || echo fallback
true && echo success
(false || true) && echo grouped
false | true
echo pstatus:$?
true | false
echo pstatus:$?
cd . | cat
echo pstatus:$?
sleep 1 &
jobs
fg 1
help
exit 0
EOF

"$BIN" < test_input.mysh > output.txt 2>&1

assert_contains() {
  local needle="$1"
  if ! grep -q "$needle" output.txt; then
    echo "assertion failed: expected output to contain '$needle'"
    echo "----- output -----"
    cat output.txt
    echo "------------------"
    exit 1
  fi
}

assert_contains "hello"
assert_contains "bar"
assert_contains "a \"quoted\" string"
assert_contains "10"
assert_contains "ok"
assert_contains "nested"
assert_contains "one"
assert_contains "two"
assert_contains "subshell:inner"
assert_contains "outer:"
assert_contains "hi"
assert_contains "add"
assert_contains "fallback"
assert_contains "success"
assert_contains "grouped"
assert_contains "pstatus:0"
assert_contains "pstatus:1"
assert_contains "cd: not supported in pipelines/subprocess context"
assert_contains "Running sleep 1"
assert_contains "Built-ins:"

echo "smoke test passed"

# Test trace modes
echo ""
echo "Testing trace modes..."

cat > trace_test.mysh <<'EOF'
echo test | wc -c
EOF

echo "--- testing basic trace (MYSH_TRACE=1) ---"
MYSH_TRACE=1 "$BIN" < trace_test.mysh 2>&1 | grep -q "TRACE" && echo "basic trace ok" || echo "basic trace failed"

echo "--- testing json trace (MYSH_TRACE=json) ---"
MYSH_TRACE=json "$BIN" < trace_test.mysh 2>&1 | grep -q '"exit_code"' && echo "json trace ok" || echo "json trace failed"

echo "--- testing graph trace (MYSH_TRACE=graph) ---"
MYSH_TRACE=graph "$BIN" < trace_test.mysh 2>&1 | grep -q "TRACE GRAPH" && echo "graph trace ok" || echo "graph trace failed"

rm trace_test.mysh
