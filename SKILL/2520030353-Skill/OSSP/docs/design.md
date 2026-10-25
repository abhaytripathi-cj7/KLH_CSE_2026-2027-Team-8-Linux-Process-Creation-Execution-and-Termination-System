# Shell architecture

```mermaid
flowchart LR
    A[Read a line] --> B[Tokenize quotes escapes variables]
    B --> C[Parse commands pipes redirections background flag]
    C --> D{One foreground builtin?}
    D -- Yes --> E[Save file descriptors then run builtin in shell]
    D -- No --> F[Fork each command and create process group]
    F --> G[Connect pipes and apply redirections in order]
    G --> H[Run builtin or execvp]
    F --> I{Foreground?}
    I -- Yes --> J[Give terminal to job and wait]
    I -- No --> K[Record job and return prompt]
    J --> L[Restore terminal to shell]
```

`parser.c` produces a bounded `Pipeline` structure with up to 16 commands, 64 arguments per command, and 16 redirections per command. `executor.c` calls `pipe`, `fork`, `setpgid`, `dup2`, `open`, `execvp`, `waitpid`, and `tcsetpgrp`. Redirections apply left to right after pipe connections, so `> file 2>&1` sends both streams to the file. For a standalone builtin, descriptors 0–2 are saved and restored around the builtin so `pwd > file` does not redirect later prompts.

`builtin.c` handles commands that change shell state in the shell process. The `cd -` command uses the previous directory. `export` validates variable names and uses `setenv`, so children inherit values. `shell.c` holds the history ring and job table. The shell ignores interactive terminal signals; children restore default signal behavior. A SIGCHLD handler sets a flag; the main loop reaps children with `waitpid` outside the handler.

Known limits are listed in the README. The shell does not attempt general POSIX shell compatibility. The parser reports malformed pipelines and redirections instead of executing a partial command.
