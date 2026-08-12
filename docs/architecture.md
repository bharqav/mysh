# Architecture Overview

This file is a copy of the top-level `ARCHITECTURE.md` to live under `docs/` for
documentation consistency.

## Execution pipeline

`mysh` follows a classic shell pipeline:

1. Read input line
2. Lex into tokens
3. Parse into executable program representation
4. Execute with POSIX process primitives

## Modules

- `core/`: shell lifecycle and REPL loop
- `lexer/`: tokenization and env expansion
- `parser/`: builds expression trees with logical precedence and grouping
- `executor/`: process creation, pipes, redirections, child/parent built-in semantics
- `builtins/`: shell-native commands
- `utils/`: environment and signal setup

## Data model

- `Token`: lexical units (`WORD`, pipe/logical/background/operators/redirects/parentheses)
- `Command`: argv + redirections
- `Pipeline`: list of commands connected by pipes
- `Expr`: expression tree node (`Pipeline`, `And`, `Or`)

## Parse strategy

Recursive descent parser with precedence:

1. parse OR expressions
2. parse AND expressions
3. parse grouped terms `( ... )`
4. parse pipelines and command redirections

This gives correct `&&` vs `||` evaluation semantics and explicit grouping.

## Process model

- Parent shell process stays alive and controls state/history/jobs.
- External commands run in child processes via `fork` + `execvp`.
- Pipes created between commands with `pipe` + `dup2`.
- Parent waits foreground jobs and tracks background jobs.

## Built-in strategy

- Standalone built-ins execute in parent process.
- Redirections for standalone built-ins are applied by temporarily remapping parent stdio and restoring descriptors after execution.
- Built-ins inside a pipeline execute in child process.
- Stateful built-ins (`cd`, `export`, `unset`, `fg`, `bg`, `exit`) fail explicitly in child/subprocess contexts.

## Signal strategy

- Interactive shell:
  - handles `SIGINT` to keep prompt loop alive
  - ignores `SIGQUIT`
- Child processes reset `SIGINT` and `SIGQUIT` to defaults before exec.

## Job control basics

- `&` marks a pipeline as background.
- Background pipelines are assigned a process group and tracked in a job table.
- Built-ins `jobs`, `fg`, and `bg` interact with the job table.

## Known limitations

- Arithmetic expansion is not implemented.
- Command substitution implementation is intentionally basic and does not aim for full Bash parity.
- Job control is intentionally basic (no `SIGTSTP`/`Ctrl+Z` workflow yet).
- No advanced wildcard/quote parity with Bash in all edge cases.
