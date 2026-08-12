#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

MYSH_BIN="${1:-./mysh}"
RUNS="${RUNS:-7}"
WARMUP="${WARMUP:-2}"
OUT_DIR="${OUT_DIR:-benchmarks}"

if [[ ! -x "$MYSH_BIN" ]]; then
  echo "error: binary '$MYSH_BIN' not found or not executable"
  exit 1
fi

mkdir -p "$OUT_DIR"

timestamp="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
os_name="$(uname -srm 2>/dev/null || echo unknown)"
cpu_model="$(uname -m 2>/dev/null || echo unknown)"

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

measure_ms() {
  local impl="$1"
  local line="$2"
  local start_ns end_ns elapsed_ns
  start_ns="$(date +%s%N)"
  if [[ "$impl" == "mysh" ]]; then
    "$MYSH_BIN" -c "$line" > /dev/null
  else
    bash -lc "$line" > /dev/null
  fi
  end_ns="$(date +%s%N)"
  elapsed_ns=$((end_ns - start_ns))
  echo $((elapsed_ns / 1000000))
}

run_impl_case() {
  local impl="$1"
  local scenario="$2"
  local line="$3"
  local -a samples=()

  for ((i=0; i<WARMUP; i++)); do
    measure_ms "$impl" "$line" >/dev/null
  done

  for ((i=0; i<RUNS; i++)); do
    samples+=("$(measure_ms "$impl" "$line")")
  done

  local median p95
  median="$(median_of "${samples[@]}")"
  p95="$(p95_of "${samples[@]}")"
  printf "%s,%s,%s,%s,%s\n" "$impl" "$scenario" "$RUNS" "$median" "$p95"
}

csv_file="$OUT_DIR/compare.csv"
md_file="$OUT_DIR/compare.md"

SCENARIOS=(
  "startup_empty:true"
  "simple_echo:echo hello"
  "pipeline_echo_wc:echo hello | wc -c"
  "logical_group:(false || true) && echo done"
)

{
  echo "impl,scenario,runs,median_ms,p95_ms"
  for pair in "${SCENARIOS[@]}"; do
    name="${pair%%:*}"
    cmd="${pair#*:}"
    run_impl_case "mysh" "$name" "$cmd"
    run_impl_case "bash" "$name" "$cmd"
  done
} > "$csv_file"

{
  echo "# Benchmark Comparison (mysh vs bash)"
  echo
  echo "- mysh binary: \`$MYSH_BIN\`"
  echo "- runs per scenario: $RUNS"
  echo "- warmup runs: $WARMUP"
  echo "- timestamp: $timestamp"
  echo "- host os: $os_name"
  echo "- host arch: $cpu_model"
  echo
  echo "| Scenario | mysh Median (ms) | bash Median (ms) | Overhead vs bash |"
  echo "|---|---:|---:|---:|"
  awk -F',' '
  NR > 1 {
    key=$2
    if ($1 == "mysh") mysh[key]=$4 + 0
    if ($1 == "bash") bsh[key]=$4 + 0
  }
  END {
    for (k in mysh) {
      if (!(k in bsh) || bsh[k] <= 0) {
        continue
      }
      overhead = ((mysh[k] - bsh[k]) / bsh[k]) * 100.0
      printf("| %s | %d | %d | %+0.1f%% |\n", k, mysh[k], bsh[k], overhead)
    }
  }' "$csv_file" | sort
} > "$md_file"

echo "benchmark comparison complete:"
echo "  $csv_file"
echo "  $md_file"
