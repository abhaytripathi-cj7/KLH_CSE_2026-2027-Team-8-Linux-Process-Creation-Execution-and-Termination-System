# Skill 7 Part B: pwd, exit, and export

`pwd` calls `getcwd`; `exit` accepts an optional numeric status and frees shell records on shutdown; `export NAME=VALUE` validates the name and uses `setenv`, so `execvp` children inherit it. Source: `../OSSP/src/builtin.c` and `../OSSP/src/shell.c`.

```text
pwd
export COURSE=OSSP
sh -c 'printf "%s\n" "$COURSE"'
exit 0
```
