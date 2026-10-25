# OSSP Skill remaining weeks: Linux shell

This project implements the remaining Skill exercises from Week 5 Part B through Week 14 in one modular C shell. Source is in `src/`; the parser, builtins, executor, and main loop have separate files. The week notes in `../Skill-5` through `../Skill-14` map each exercise to commands you can run.

## Build and use

```sh
make
./ossp-shell
make test
```

Example session:

```text
ossp> export GREETING=hello
ossp> echo "$GREETING world" | tr a-z A-Z > greeting.txt
ossp> cat < greeting.txt
HELLO WORLD
ossp> sleep 30 &
[1] 12345
ossp> jobs
ossp> fg %1
```

Supported features: `$NAME` and `${NAME}` expansion (including nested environment values, limited to eight levels), single and double quotes, backslash escaping, `cd`, `pwd`, `export`, `exit`, `history`, `jobs`, `fg`, `bg`, pipelines, `<`, `>`, `>>`, `2>`, `2>>`, `2>&1`, and `&`. In an interactive terminal, foreground process groups receive Ctrl-C and Ctrl-Z, while the shell keeps running.

The shell is an educational subset. It does not implement globbing, command substitution, `&&`, `||`, `;`, or persistent history. Each input line is one pipeline. History keeps the last 100 commands for the current session. Background job records hold up to 64 jobs and are cleared after a `jobs` display reports completion.

`make test` runs isolated tests. `tests/job_control.py` exercises terminal signals using a pseudo-terminal. The `scripts/run.sh` helper builds and starts the shell. See `docs/design.md` for the process and descriptor flow.
