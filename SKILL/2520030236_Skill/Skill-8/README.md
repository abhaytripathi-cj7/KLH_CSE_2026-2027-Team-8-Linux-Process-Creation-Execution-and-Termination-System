# Skill 8: history and pipelines

`../OSSP/src/shell.c` stores the latest 100 lines in a ring buffer and the `history` builtin displays them. `../OSSP/src/parser.c` stores pipeline stages in execution order; `../OSSP/src/executor.c` uses `pipe`, `dup2`, `fork`, `execvp`, and `waitpid`, closing unused descriptors.

```text
echo one
history
printf 'hello\n' | tr a-z A-Z
printf 'hello\n' | cat | cat | tr a-z A-Z
```

Run `make -C ../OSSP test` to verify a five-stage pipeline. The table has a 16-command limit.
