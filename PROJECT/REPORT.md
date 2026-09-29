# Project report: Linux process lifecycle using a payment simulation

## Aim

Demonstrate process creation, execution, input waiting, signal termination,
normal exit and status collection using a payment workflow implemented in C
on Linux. Display actual process identifiers and operating-system information
at each observable stage.

## Problem statement

A payment application starts a separate process when a customer chooses to
pay. The payment process executes a worker program and waits for an OTP. A
correct OTP completes the payment and exits normally. Decline terminates the
worker by signal. The application collects the child process status and reports
the outcome. Incorrect OTP provides an additional example of normal exit with
a nonzero status.

## Process architecture

```text
Linux shell
    |
    +-- Payment application (parent PID = P; PPID = shell PID)
            |
            | fork()
            v
        Payment child (PID = C; PPID = P)
            |
            | execl(): same PID C, new program image
            v
        Payment worker
            |
            | read(OTP pipe): blocks while pipe is empty
            +-----------------+------------------+
            |                 |                  |
       OTP 123456        Incorrect OTP         Decline
            |                 |                  |
       normal exit 0     normal exit 1      SIGTERM from P
            +-----------------+------------------+
                              |
                         Zombie state Z
                              |
                   Parent waitpid() reaps C
                              |
                   Child process entry released
```

`P` and `C` are explanatory placeholders; the application prints real numeric
values. The parent remains alive until its child has been reaped.

## Implementation

**Parent:** accepts the payment mode and amount, creates two pipes and calls
`fork()`. It sends the entered OTP through one pipe, or sends SIGTERM on decline.
It reads final child status and prints the payment process outcome.

**Child:** closes unused pipe ends, displays its PID/PPID, and calls `execl()`
using `/proc/self/exe` with a `--worker` argument. This re-executes the same
executable in worker mode, replacing its process image without creating another
process. The worker displays its identity, signals readiness through the second
pipe and blocks reading the OTP pipe.

**Synchronization:** the readiness pipe ensures the parent does not display
the OTP prompt or send decline before the worker has initialized. The parent
also samples the child's state briefly to observe the input-blocked state.

**Cleanup:** unused file descriptors are closed. Input EOF at the OTP prompt
is treated as decline. The parent uses `waitid(WEXITED | WNOWAIT)` to observe
termination without reaping, displays the zombie, then calls `waitpid()` to
reap the child. Interrupted reads and waits are retried.

## Algorithm

1. Display the parent process information.
2. Read and validate payment mode and amount.
3. When the user enters `pay`, create OTP and readiness pipes.
4. Call `fork()` and branch into parent and child paths.
5. In the child, display process information and execute worker mode.
6. In the worker, report readiness and block on OTP input from the parent.
7. In the parent, display the waiting child's information and request an action.
8. On OTP input, write it to the pipe. On decline, send SIGTERM to the child.
9. The worker exits with `0` on correct OTP or `1` on incorrect OTP.
10. The parent waits for termination, displays the zombie and reaps the child.
11. Decode the wait status to distinguish normal exit from signal termination.

## Process information glossary

| Field | Meaning |
|---|---|
| PID | Unique identifier of a current process; may be reused after its lifetime |
| PPID | PID of its parent process |
| State `R` | Running or runnable |
| State `S` | Interruptible sleep, including waiting for pipe input |
| State `Z` | Exited process whose status has not yet been collected |
| PGID | Process group identifier, used in job control and group signaling |
| SID | Session identifier, associated with terminal/session organization |
| UID | User identifiers controlling process identity and permissions |
| Threads | Number of threads in the process |
| VmRSS | Resident memory currently in RAM, reported in kB by Linux |
| Nice | Scheduling preference; lower values favor CPU allocation under normal policies |
| Raw wait status | Encoded result returned by `waitpid()`; use macros to decode it |
| Exit status | Application-supplied normal result, examined only when `WIFEXITED` is true |
| Termination signal | Signal that ended the child, examined only when `WIFSIGNALED` is true |

Running states are observations at a particular instant. A process does not
necessarily remain `R` for a whole business stage, and a successful payment
does not have a permanent Linux state named "success".

## System interfaces used

| Interface | Role |
|---|---|
| `getpid()`, `getppid()` concept | Identify self and parent; snapshots read PID/PPID from `/proc` |
| `pipe()` | One-way parent/child communication |
| `fork()` | Create child; returns child PID to parent and zero to child |
| `execl()` | Replace child image with worker mode |
| `read()`, `write()` | Exchange readiness and OTP data |
| `kill()` | Send SIGTERM to the payment child |
| Return from `main()` | Normal C program termination, equivalent to calling `exit()` with that value |
| `_exit(127)` | End the child if `exec` fails |
| `waitid()` | Wait for termination while retaining the child's waitable status |
| `waitpid()` | Collect child status and reap it |
| `/proc/PID/status` | Inspect live kernel process information |
| `getpgid()`, `getsid()`, `getpriority()` | Read group, session and nice information |

## Expected outcomes

| Scenario | Waiting state | End mechanism | Decoded child result |
|---|---|---|---|
| Correct OTP `123456` | `S` | Normal exit | `WIFEXITED=true`, exit code `0` |
| Incorrect OTP | `S` | Normal exit | `WIFEXITED=true`, exit code `1` |
| Decline | `S` | SIGTERM default action | `WIFSIGNALED=true`, signal `15` |

All three scenarios briefly expose state `Z`, then reap the child. PID and PPID
vary between runs and machines. The exit code of the parent demonstration is
separate from the exit code of the payment worker.

## Testing

Run `make test`. Integration checks verify the three scenarios, PID preservation
across exec, child/parent identity, sleeping and zombie states, reaping, all three
interactive payment modes, decline, input EOF and invalid amount handling.
Captured logs and compiler/test output are included with the project.

## Viva questions

**Does exec create a process?** No. `fork()` creates it; exec replaces its image
while preserving its PID.

**Why is the child sleeping during OTP entry?** Its blocking read has no input
available. The kernel can run other processes while it waits.

**Is OTP waiting the same as waitpid?** No. OTP waiting is blocked I/O in the
child. `waitpid()` is used by the parent to collect its child's result.

**Does kill always kill immediately?** No. It sends a signal. Here SIGTERM has
its default action in the worker, which terminates it. Other programs may catch
or ignore SIGTERM.

**Are termination and exit different?** Exit is one way of terminating. This
project contrasts normal exit with termination caused by a signal.

**What is a zombie?** A terminated child retaining minimal kernel information
so its parent can collect its result. It is not an executing payment worker.

**Why call waitpid?** To obtain the child's result and release its zombie entry.

**What does PPID show after fork?** The payment child points to the payment
application's PID. The application's own PPID usually identifies its shell.

## Conclusion

The payment example connects user actions with real Linux process behavior.
It demonstrates creation through fork, image replacement through exec, blocked
input, normal and signal-based termination, and parent-controlled reaping.
