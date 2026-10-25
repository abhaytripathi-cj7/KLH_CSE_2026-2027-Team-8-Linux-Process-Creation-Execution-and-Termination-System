# Skill 14: final shell demonstration

Build with `make -C ../OSSP`, then run `../OSSP/ossp-shell`. Source structure, build instructions, and the process flow diagram are in `../OSSP/README.md` and `../OSSP/docs/design.md`. The `scripts/run.sh` helper builds and launches the demo. The repository commit records the delivered version; no release tag is created by the demo.

Suggested presentation:

```text
export MESSAGE=ready
echo "$MESSAGE" | tr a-z A-Z | cat
echo first > log.txt
echo second >> log.txt
cat < log.txt
sleep 30 &
jobs
fg
```

Press Ctrl-Z during a foreground `sleep`, then use `bg` and `fg` to demonstrate job control. Press Ctrl-C to end the foreground job. For stability, run `make -C ../OSSP test`, including the long pipeline and signal checks.
