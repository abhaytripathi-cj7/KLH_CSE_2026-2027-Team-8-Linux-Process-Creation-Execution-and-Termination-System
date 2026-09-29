# ProcessPay — Linux Process Lifecycle

A C mini-project demonstrating Linux process **creation, execution, waiting,
signal termination, normal exit and reaping** through a simulated payment.
It uses real Linux processes and live `/proc` information.

**Live website:** https://processpay-linux-lab-jashwanth.netlify.app

The frontend is hosted on Netlify. Its real Linux backend currently runs through
a temporary tunnel, so payment actions are available while the backend computer
and tunnel are running. The project can also run entirely on localhost.

**Demo OTP:** `123456`.

## Website on localhost

The complete website is included. Run `python3 server.py` on Linux or
double-click `start-website.bat` on Windows with Ubuntu WSL, then open
**http://localhost:8000**. See [WEBSITE.md](WEBSITE.md) for instructions.

## Run

Requirements: Linux (or Ubuntu on WSL), GCC, Make; Python 3 for tests only.
On Ubuntu, if the tools are missing: `sudo apt install build-essential python3`.

Open a terminal inside this project directory:

```bash
make
./payment
```

1. Select UPI, Card or Net banking.
2. Enter an amount, such as `500`.
3. Type `pay` to create the payment child.
4. Enter demo OTP `123456` for success, any other OTP for failure, or
   `decline` to terminate the child with SIGTERM.

The modes are labels for the same process demonstration. No real money,
account, card details, network request or production OTP service is involved.

## One-command demonstrations

```bash
./payment --demo success
./payment --demo decline
./payment --demo wrong-otp
make test
```

## What each payment action demonstrates

| Payment action | Linux operation | Observable result |
|---|---|---|
| Start application | Shell launches parent | Parent PID and shell PPID |
| Press Pay | `fork()` | Child gets a new PID; its PPID is the parent PID |
| Run payment worker | `execl()` | Program image replaced; child PID stays the same |
| Await OTP | Child `read()` on an empty pipe | Child normally has state `S` (interruptible sleep) |
| Enter OTP | Parent `write()` to pipe | Child becomes runnable and validates OTP |
| Correct OTP | Worker returns `0` from `main()` | Normal exit, `WEXITSTATUS=0` |
| Incorrect OTP | Worker returns `1` from `main()` | Normal failure exit, `WEXITSTATUS=1` |
| Decline | Parent `kill(child, SIGTERM)` | Signal termination, `WTERMSIG=15` on Linux |
| Observe terminated child | `waitid(..., WNOWAIT)` | Child is a zombie (`Z`) until reaped |
| Collect final result | `waitpid()` | Exit/signal status decoded; child is reaped |

Both success and decline are forms of process termination. This project uses
**normal exit** for success and **signal termination** for decline to show the
difference. OTP waiting is input waiting, not a call to `wait()` by the child.

## Information displayed

Each process snapshot shows PID, PPID, observed state, program name, UID values
(real, effective, saved and filesystem), thread count, resident memory when
available, process group ID (PGID), session ID (SID), and nice value.
The final result shows the reaped PID, raw wait status, decoded exit code or
terminating signal. PID and PPID are taken from the real running processes.

For an independent view, leave the program at the OTP prompt, open another
Linux terminal, and replace `CHILD_PID` with the printed child PID:

```bash
ps -o pid,ppid,pgid,sid,stat,ni,rss,comm -p CHILD_PID
cat /proc/CHILD_PID/status
```

State is sampled, so short running stages can vary with scheduling. The program
waits briefly for the child to enter `S` before showing the OTP snapshot.
The zombie snapshot is deliberate and immediately followed by reaping.
Memory fields may be absent after the process has exited.

## Files

- `payment.c`: implementation and comments.
- `Makefile`: build and test commands.
- `tests.py`: integration tests against actual Linux processes.
- `REPORT.md`: project explanation, algorithm and viva questions.
- `sample-success.txt`, `sample-decline.txt`, `sample-wrong-otp.txt`: captured runs.
- `validation.txt`: compiler and integration-test results.

The parent returns `0` when it completes the demonstration, even for a declined
payment. The **child's** result is printed separately. Invalid setup input makes
the parent return `2`; failed child execution produces child exit code `127`.

This is a single-payment teaching project. OTP timeout, retries, persistent
transactions and handling forced termination of the parent are outside its scope.
