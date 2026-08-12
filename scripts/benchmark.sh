#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

BIN="${1:-./mysh}"
RUNS="${RUNS:-7}"
WARMUP="${WARMUP:-2}"
OUT_DIR="${OUT_DIR:-benchmarks}"

if [[ ! -x "$BIN" ]]; then
  echo "error: binary '$BIN' not found or not executable"
  exit 1
fi

mkdir -p "$OUT_DIR"
tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

timestamp="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
os_name="$(uname -srm 2>/dev/null || echo unknown)"
cpu_model="$(uname -m 2>/dev/null || echo unknown)"
compiler_version="$(
  g++ --version 2>/dev/null | awk 'NR==1 {print; exit}' || echo "g++ unavailable"
)"

bench_file() {
  local name="$1"
  local content="$2"
  local f="$tmpdir/${name}.mysh"
  printf "%s\nexit 0\n" "$content" > "$f"
  echo "$f"
}

startup_script="$(bench_file startup '')"
simple_script="$(bench_file simple 'echo hello')"
pipeline_script="$(bench_file pipeline 'echo hello | wc -c')"
logic_script="$(bench_file logic '(false || true) && echo done')"
redir_script="$(bench_file redir 'echo data > out.txt; cat < out.txt')"
glob_script="$(bench_file glob 'echo x > a.cpp; echo x > b.cpp; echo *.cpp')"

measure_ms() {
  local script="$1"
  local start_ns end_ns elapsed_ns
  start_ns="$(date +%s%N)"
  "$BIN" < "$script" > /dev/null
  end_ns="$(date +%s%N)"
  elapsed_ns=$((end_ns - start_ns))
  echo $((elapsed_ns / 1000000))
}

median_of() {
  printf "%s\n" "$@" | sort -n | awk '{
    a[NR]=$1
  } END {
    if (NR==0) { print 0; exit }
    if (NR%2==1) print a[(NR+1)/2]
    else print int((a[NR/2] + a[NR/2+1]) / 2)
  }'
}

p95_of() {
  printf "%s\n" "$@" | sort -n | awk '{
    a[NR]=$1
  } END {
    if (NR==0) { print 0; exit }
    idx = int((NR * 95 + 99) / 100)
    if (idx < 1) idx = 1
    if (idx > NR) idx = NR
    print a[idx]
  }'
}

run_case() {
  local name="$1"
  local script="$2"
  local -a samples=()

  for ((i=0; i<WARMUP; i++)); do
    measure_ms "$script" >/dev/null
  done

  for ((i=0; i<RUNS; i++)); do
    samples+=("$(measure_ms "$script")")
  done

  local median p95
  median="$(median_of "${samples[@]}")"
  p95="$(p95_of "${samples[@]}")"
  printf "%s,%s,%s,%s\n" "$name" "$RUNS" "$median" "$p95"
}

csv_file="$OUT_DIR/results.csv"
md_file="$OUT_DIR/results.md"

{
  echo "scenario,runs,median_ms,p95_ms"
  run_case "startup_empty" "$startup_script"
  run_case "simple_echo" "$simple_script"
  run_case "pipeline_echo_wc" "$pipeline_script"
  run_case "logical_group" "$logic_script"
  run_case "redir_write_read" "$redir_script"
  run_case "glob_expand" "$glob_script"
} > "$csv_file"

{
  echo "# Benchmark Results"
  echo
  echo "- binary: \`$BIN\`"
  echo "- runs per scenario: $RUNS"
  echo "- warmup runs: $WARMUP"
  echo "- timestamp: $timestamp"
  echo "- host os: $os_name"
  echo "- host arch: $cpu_model"
  echo "- compiler: $compiler_version"
  echo
  echo "| Scenario | Runs | Median (ms) | P95 (ms) |"
  echo "|---|---:|---:|---:|"
  awk -F',' 'NR>1 { printf("| %s | %s | %s | %s |\n", $1, $2, $3, $4) }' "$csv_file"
} > "$md_file"

echo "benchmark complete:"
echo "  $csv_file"
echo "  $md_file"
