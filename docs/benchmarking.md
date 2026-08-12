# Benchmarks

This project includes a reproducible benchmark harness.

## Run benchmarks

```bash
make benchmark
```

To compare runtime overhead against `bash`:

```bash
make benchmark-compare
```

This produces:

- `benchmarks/results.csv`
- `benchmarks/results.md`
- `benchmarks/compare.csv`
- `benchmarks/compare.md`

To enforce regressions against baseline:

```bash
make benchmark-guard
```

Inputs for guard:

- baseline: `benchmarks/baseline.csv`
- current: `benchmarks/results.csv`
- threshold env var: `BENCH_REGRESSION_PCT` (default `25`)

Quick publish flow:

1. Run `make benchmark`
2. Open `benchmarks/results.md`
3. Copy the table and environment lines into `README.md` under a "Benchmark Results" section

## What is measured

- `startup_empty`: shell launch and clean exit
- `simple_echo`: single command dispatch cost
- `pipeline_echo_wc`: pipeline orchestration overhead
- `logical_group`: parser + logical expression execution overhead
- `redir_write_read`: redirection file I/O path
- `glob_expand`: wildcard expansion path

Comparison mode (`benchmark-compare`) measures a shared subset in both shells:

- `startup_empty`
- `simple_echo`
- `pipeline_echo_wc`
- `logical_group`

## Methodology

- Warmup runs are executed before timed runs.
- Median and p95 are reported to reduce noise sensitivity.
- Benchmarks should be run on an idle machine and repeated after changes.

## Recommended reporting format (README)

Include:

- host CPU / RAM / OS
- compiler version and build flags
- run count and warmup count
- median/p95 table from `benchmarks/results.md`

## Example command overrides

```bash
RUNS=15 WARMUP=3 OUT_DIR=benchmarks make benchmark
```
