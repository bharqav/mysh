# Changelog

All notable changes are documented in this file.

## [Unreleased]

### Added
- Expression-tree parser with precedence and grouping.
- Basic job control commands: `jobs`, `fg`, `bg`.
- Readline/history support toggle (`READLINE=0` fallback).
- Docker demo container and CI workflow.
- Operations/deployment/security docs.
- Regression test suite script.
- Unit test binary (`make unit`) with lexer/parser/executor checks.
- Edge integration test suite (`make edge`).
- Lint/static-analysis (`make lint`) and coverage (`make coverage`) targets.
- Benchmark baseline + regression guard (`make benchmark-guard`).
- Governance docs: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `LICENSE`.

### Changed
- Hardened signal handling with async-safe prompt write path.
- Improved process-group and foreground terminal handoff logic.
- Improved background reaping and job-state tracking (`Running`/`Stopped`/`Done`).
- Pipeline exit status now follows rightmost command semantics.
- Trace JSON output now escapes command/description content.
- Command substitution execution now uses a portable in-process child execution path.
- Stateful built-ins now fail explicitly when run in subprocess/pipeline contexts.
- CI now runs Linux/macOS matrix, edge+unit tests, lint, sanitizer runtime checks, coverage, and benchmark guard.
