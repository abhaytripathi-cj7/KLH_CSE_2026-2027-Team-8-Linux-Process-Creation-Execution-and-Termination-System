# Skill 10: combined redirection and background jobs

Redirections apply from left to right, so `> combined.txt 2>&1` combines output and errors. A pipeline may also use redirection on a stage. The trailing `&` records a process group and returns the prompt while the job runs. `../OSSP/src/executor.c` and `../OSSP/src/shell.c` maintain up to 64 job records.

```text
sh -c 'echo output; echo error >&2' > combined.txt 2>&1
cat < combined.txt | tr a-z A-Z > upper.txt
sleep 5 &
jobs
```
