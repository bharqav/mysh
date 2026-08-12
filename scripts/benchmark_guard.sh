#!/usr/bin/env bash
set -euo pipefail

BASELINE_FILE="${BASELINE_FILE:-benchmarks/baseline.csv}"
CURRENT_FILE="${CURRENT_FILE:-benchmarks/results.csv}"
ALLOW_PCT="${BENCH_REGRESSION_PCT:-25}"

if [[ ! -f "$BASELINE_FILE" ]]; then
  echo "baseline file '$BASELINE_FILE' not found"
  exit 1
fi

if [[ ! -f "$CURRENT_FILE" ]]; then
  echo "current benchmark file '$CURRENT_FILE' not found"
  exit 1
fi

awk -F',' -v allow_pct="$ALLOW_PCT" '
FNR==NR {
  if (NR > 1) {
    base[$1] = $3
  }
  next
}
NR > 1 {
  scenario = $1
  current = $3 + 0
  if (!(scenario in base)) {
    printf("missing baseline scenario: %s\n", scenario) > "/dev/stderr"
    failed = 1
    next
  }
  baseline = base[scenario] + 0
  if (baseline <= 0) {
    next
  }
  allowed = baseline * (1 + allow_pct / 100.0)
  if (current > allowed) {
    printf("regression: %s baseline=%s current=%s allowed=%0.2f\n", scenario, baseline, current, allowed) > "/dev/stderr"
    failed = 1
  }
}
END {
  if (failed) {
    exit 1
  }
}
' "$BASELINE_FILE" "$CURRENT_FILE"

echo "benchmark guard passed (threshold ${ALLOW_PCT}%)"
