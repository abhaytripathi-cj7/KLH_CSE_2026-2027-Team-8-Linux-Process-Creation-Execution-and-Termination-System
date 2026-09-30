# Skill 12: signal actions and process groups

`../OSSP/src/shell.c` installs a SIGCHLD action and ignores interactive SIGINT and SIGTSTP in the shell. Children restore default handlers. `../OSSP/src/executor.c` assigns each pipeline one process group with `setpgid`, transfers terminal ownership with `tcsetpgrp`, and sends SIGCONT to resume stopped groups.

Run `python3 ../OSSP/tests/job_control.py` after building. It verifies Ctrl-C interrupts a foreground command without exiting the shell, Ctrl-Z stops it, `bg` resumes it, and `fg` returns it to the terminal.
