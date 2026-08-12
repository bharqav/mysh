# Operations Guide

## Runtime modes

- Interactive mode: `./mysh`
- Script mode: `./mysh < script.mysh`

## Build profiles

- Release: `make`
- Debug symbols: `make debug`
- Sanitizers: `make asan`
- No readline: `make READLINE=0`

## Health checks

- Smoke test: `make test`
- Regression suite: `make regression`
- Edge suite: `make edge`
- Unit suite: `make unit`
- Combined checks: `make check`
- Benchmark run: `make benchmark`
- Benchmark guard: `make benchmark-guard`
- Lint/static analysis: `make lint`
- Coverage report: `make coverage`
- CI pipeline validates build + smoke + regression + no-readline + sanitizer compile.
- Benchmark report includes environment metadata and median/p95 latencies.

## Job control notes

- Background pipelines are started with `&`.
- Use `jobs` to inspect state.
- Use `fg <id>` or `bg <id>` to control jobs.
- Job states are reported as `Running`, `Stopped`, and `Done`.
- Stateful built-ins (`cd`, `export`, `unset`, `fg`, `bg`, `exit`) require parent-shell context and fail in pipeline/subprocess contexts.

## Failure handling

- Parser errors return status `2`.
- Execution failures follow POSIX-like status behavior (`127` for exec failure).
- Signals in foreground children map to `128 + signal`.

## Known operational limitations

- Job control is intentionally basic compared to full Bash job semantics.
- Heredoc and expansion semantics are intentionally simplified.
- Quoting/globbing behavior is close to shell workflows but not full Bash parity.
