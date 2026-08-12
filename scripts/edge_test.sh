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

cat > edge_input.mysh <<'EOF'
false | true
echo status:$?
true | false
echo status:$?

echo "x\"y"
export EDGE=ok
echo "pre-$EDGE-post"

__definitely_not_a_real_command__
echo status:$?

cat < does_not_exist.txt
echo status:$?

export 1BAD=value
echo status:$?
unset 1BAD
echo status:$?

cd . | cat
echo status:$?

echo $(echo $(echo deep))

exit 0
EOF

"$BIN" < edge_input.mysh > output.txt 2>&1

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

assert_contains "status:0"
assert_contains "status:1"
assert_contains "x\"y"
assert_contains "pre-ok-post"
assert_contains "status:127"
assert_contains "status:1"
assert_contains "export: '1BAD': not a valid identifier"
assert_contains "unset: '1BAD': not a valid identifier"
assert_contains "cd: not supported in pipelines/subprocess context"
assert_contains "deep"

echo "edge test passed"
