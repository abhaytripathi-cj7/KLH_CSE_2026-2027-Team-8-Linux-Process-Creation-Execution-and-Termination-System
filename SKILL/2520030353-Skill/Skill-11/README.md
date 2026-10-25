# Skill 11: jobs, foreground, background, and signals

`jobs` lists running, stopped, and completed jobs. `fg %ID` transfers terminal control to a job process group, resumes it if stopped, waits, and restores the shell terminal. `bg %ID` sends SIGCONT without taking the terminal. The shell reaps status changes with `waitpid` outside its SIGCHLD handler.

In a terminal, run `sleep 30`, press Ctrl-Z, then enter `jobs`, `bg`, `jobs`, and `fg`. Press Ctrl-C to end the foreground sleep. `make -C ../OSSP test` also runs an automated pseudo-terminal test of this sequence.
