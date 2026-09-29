# ProcessPay localhost website

Open **http://localhost:8000** while the server is running.

## Start on your Windows computer

Open the project folder and double-click `start-website.bat`.
This uses your installed Ubuntu WSL environment. Keep its terminal open, then
open the address above in any browser. Press Ctrl+C in that terminal to stop.
If a server is already running on port 8000, use that server or stop it first.

## Start directly on Linux / Ubuntu

Inside the extracted project folder, run:

```bash
python3 server.py
```

Python 3, GCC and Make are required. No pip, npm or external packages are needed.
The server compiles the C program automatically when needed. If moving the
project to another Linux architecture, run `make clean` before starting it.

## Use the website

1. Select UPI, Card or Bank and enter a whole-number amount.
2. Click **Pay & create process**.
3. Watch the parent and child PID/PPID appear in the process inspector.
4. Enter **123456** and click **Verify & complete payment** for a normal exit.
5. Alternatively, click **Decline payment** for SIGTERM termination, or enter
   a different six-digit OTP for a failure exit.
6. Read the execution trace and expand the raw process output for full details.
7. Click **Start another payment** to repeat the demonstration.

## Architecture

```text
Browser (HTML / CSS / JavaScript)
    | localhost HTTP + JSON, refreshed every 500 ms
Python local server
    | starts C payment application, pipes its input and output
C payment application (parent PID)
    | fork + exec
C payment worker (child PID, PPID = application PID)
    | waits for OTP, exits normally or terminates by signal
Parent observes zombie and reaps child with waitpid()
```

The payment application is the worker's parent; its own PPID identifies the
Python server. The dashboard reads actual Linux `/proc` information. After
reaping, it labels the retained information as a final snapshot. A reaped
process no longer has a live kernel process entry. Memory can be absent from
the zombie snapshot.

The mode and amount are educational inputs. No money moves and no banking
services are contacted. A fixed demo OTP is intentional.

The website binds to loopback only. One payment is active at a time across
all local tabs; a reload reconnects to the current demonstration. Closing a
browser tab does not cancel a payment. Use Decline or stop the server normally
to clean up an active worker. State and logs are held in memory and reset when
the server restarts. Forced OS shutdown is outside the demonstration's cleanup
guarantees.

## Files and checks

- `server.py`: local HTTP API and process bridge.
- `dist/index.html`, `dist/style.css`, `dist/app.js`: responsive website.
- `payment.c`: real Linux process lifecycle.
- `start-website.bat`: Windows/WSL launcher.
- `README.md`, `REPORT.md`: C project instructions and OS explanation.

With the website running and no payment in progress:

```bash
python3 web_tests.py
```

The tests create demo payments, verify success/failure/decline, input validation,
PID/PPID relationships, sleeping, signal status and process cleanup. They leave
the final tested outcome visible. `make test` runs the original C integration tests.

The page optionally registers structured browser tools when WebMCP is available.
Normal browser operation does not depend on this feature.
