# PipeTalk — Inter-Process Chat Application Using IPC

**OSSP Hackathon · Team 8**

A web application demonstrating two-way communication between independent Linux processes using named pipes. A browser presents two chat endpoints; two C programs exchange the actual message bytes through Linux FIFO special files.

## Requirements

- Ubuntu 22.04 or newer, either native or through WSL 2 on Windows 10/11.
- Python 3.10+ and GCC. The launcher installs missing prerequisites through Ubuntu's package manager. Installation requires internet access and may ask for your Ubuntu password.
- A modern browser: Chrome, Edge or Firefox.
- No Node.js, npm packages, pip packages, database or external service is required. Once Python and GCC are installed, the application runs offline.

## Windows: one-click start

1. Extract the ZIP fully into a normal local folder. Do not run from inside the ZIP preview.
2. Double-click **START_WINDOWS.bat**.
3. The launcher selects an installed Ubuntu WSL distribution, starts the Linux backend, and opens the browser. If dependencies are missing, allow the Ubuntu setup to finish. A sudo password is your Ubuntu password; its characters are not displayed while typing.
4. Keep the terminal open while using the application.

If Ubuntu/WSL is not installed, perform this one-time setup in Administrator PowerShell:

```powershell
wsl --install -d Ubuntu
```

Restart Windows if requested. Open Ubuntu once, create its username/password, then run START_WINDOWS.bat again. The batch file cannot complete Windows feature activation or Ubuntu account creation for you.

If Windows blocks the script or the organization disables PowerShell, open Ubuntu manually, change to the extracted folder and run `bash start.sh`.

Example for a Windows Downloads folder (replace the username and folder location):

```bash
cd "/mnt/c/Users/YOUR_USERNAME/Downloads/PipeTalk-Team8"
bash start.sh
```

## Native Ubuntu

Open a terminal in the extracted project folder:

```bash
bash START_UBUNTU.sh
```

Alternatively mark START_UBUNTU.sh executable and choose **Run as a Program** in a file manager that supports this option. Terminal launch is the most portable method.

The default address is **http://localhost:8080**. If that port is occupied, the server chooses an available port through 8099 and prints the exact URL. If the browser does not open automatically, open the printed URL yourself.

To choose a different starting port:

```bash
bash start.sh --port 8090
```

To stop, press **Ctrl+C** in the launcher terminal. The bridge terminates and reaps both peers, closes descriptors and removes its temporary FIFO directory. Abrupt termination with SIGKILL or a system crash can leave a temporary directory; subsequent starts use a fresh unique directory.

## Using the application

1. In **Live chat**, type in Process A and press Enter or the arrow button.
2. The message appears at A as queued, then at B after its C process confirms receipt.
3. Reply from Process B to demonstrate the reverse direction.
4. **Transport activity** shows queued, write, read, ready and offline events with timestamps and PIDs. Scheduling can cause a read event to reach the bridge before the corresponding write event.
5. **Process inspector** shows real PIDs, process states, RSS and descriptor counts read from `/proc`, together with actual FIFO paths and commands to inspect them.
6. Use **Restart processes** to recreate both peers and FIFO channels without restarting the website. New PIDs will appear; the current bridge session history remains.
7. **Export session** downloads JSON containing retained messages and events.

Each message is limited to **1600 UTF-8 bytes**. Newlines, quotes, Unicode and HTML-like text are transported as data. Control characters that cause a JSON frame to exceed 4096 bytes are rejected. There are at most 100 outstanding messages; a message with no receiving confirmation within five seconds is displayed as failed. Very delayed frames can subsequently be confirmed as delivered if the peer resumes and reads them.

The bridge retains the latest **500 messages** and **200 events** in memory; older entries are discarded. Session counters cover the entire bridge run. History is not saved across server restarts. Export before stopping.

## Linux backend demonstration

Keep the browser beside an Ubuntu terminal. While the application is running:

```bash
bash scripts/inspect.sh
```

If an alternate port was printed:

```bash
bash scripts/inspect.sh 8090
```

This prints the actual PIDs and FIFO directory. Copy the generated commands, or use the live commands in **Process inspector**:

```bash
# Show the independent peer processes
ps -eo pid,ppid,stat,comm,args | grep '[p]ipetalk_peer'

# Inspect the actual runtime directory shown by the application
ls -l /tmp/pipetalk-REPLACE_WITH_ACTUAL_ID/*.fifo
```

The leading `p` in the file permissions identifies a named pipe. Two different PIDs identify the two independent peer processes. Send a message in each direction and show the matching receive events. Do not use `cat` on an active FIFO during the demonstration: an additional reader can consume a frame intended for a chat peer.

For failure recovery, use the actual Process B PID shown on the dashboard:

```bash
kill -TERM REPLACE_WITH_PROCESS_B_PID
```

The application reports the peer offline and blocks new sends. Select **Restart processes**, then exchange messages again.

## Architecture and implementation

```text
Browser → local HTTP bridge → anonymous stdin pipe → C peer A
C peer A → a_to_b.fifo → C peer B
C peer B → stdout confirmation → local HTTP bridge → browser
Replies: C peer B → b_to_a.fifo → C peer A
```

The Python bridge creates FIFO files with `os.mkfifo()` (the Python binding to the Linux FIFO creation operation). It launches each C peer with `subprocess.Popen()`. The peers are independent Linux processes with separate address spaces. Their stdin/stdout streams are anonymous control pipes connecting them to the bridge; the two named FIFOs are the peer-to-peer chat transport.

`backend/peer.c` uses `open()`, `poll()`, `read()`, `write()` and `close()`. Linux `O_RDWR | O_NONBLOCK` opens avoid FIFO startup ordering deadlocks. Each direction has one writer and one receiving peer; JSON newline frames are bounded to Linux PIPE_BUF (4096 bytes), allowing atomic writes. `poll()` waits for readiness without busy looping. Messages are never executed as commands.

The bridge matches destination-process receive events with submitted message IDs and text before marking delivery. The observed latency measures bridge enqueue to receipt of the C read confirmation, including scheduling overhead. It is not a kernel-only IPC benchmark. Browser updates use HTTP polling every 500 ms; there is no WebSocket dependency.

The server binds to **127.0.0.1**. It checks the Host header, protects modifying requests with a session token and rejects foreign origins. This is a local educational application rather than an authenticated public messaging service. Access from another phone or computer is outside this version's scope. Browser tabs on the same server share the same two endpoints and session.

## Source layout

| File | Responsibility |
|---|---|
| `START_WINDOWS.bat` | Windows entry point |
| `scripts/start_windows.ps1` | Detect Ubuntu on WSL and launch backend |
| `START_UBUNTU.sh`, `start.sh` | Linux entry points and prerequisite checks |
| `scripts/setup_ubuntu.sh` | Install Python/GCC if missing |
| `backend/peer.c` | Actual FIFO peer process implementation |
| `backend/server.py` | HTTP bridge, compilation, process lifecycle and transport events |
| `frontend/index.html` | Dashboard structure |
| `frontend/style.css` | Responsive dashboard styles |
| `frontend/app.js` | Chat interface, polling, inspector and export interactions |
| `scripts/inspect.sh` | Terminal inspection helper |
| `tests/test_integration.py` | Real-process integration tests |
| `tests/test_launcher.py` | Linux startup, busy-port fallback and cleanup tests |
| `tests/test_windows_launcher.py` | WSL command argument contract checks |
| `Makefile` | Optional run, test and check shortcuts |

The C program is compiled into a private `/tmp/pipetalk-*` directory on each server start. No prebuilt executable is needed. Source paths with spaces are supported.

## Tests and compilation checks

From the project folder in Ubuntu:

```bash
python3 -m unittest discover -s tests -v
```

The tests compile and launch real C processes. They cover FIFO types, distinct PIDs, message delivery in both directions, Unicode/multiline payloads, validation, request protection, concurrent sends, session export, peer failure and restart. They also exercise startup from a path containing spaces, busy-port fallback, shutdown cleanup and the WSL direct-execution argument contract. The test server uses an automatically allocated port. WSL argument checks on Linux do not replace executing the Windows launcher on Windows.

Optional checks:

```bash
make check
make test
```

To compile the C source manually for inspection:

```bash
gcc -std=c11 -O2 -Wall -Wextra -Werror backend/peer.c -o /tmp/pipetalk_peer
```

Running the peer alone requires two existing FIFO paths and a bridge-compatible JSON input stream. Use the normal launcher for the complete application.

## Troubleshooting

| Symptom | Action |
|---|---|
| Ubuntu distribution missing | Install Ubuntu with `wsl --install -d Ubuntu`, then finish its first-run setup. |
| sudo password prompt | Enter the Ubuntu password created during setup. Nothing is displayed while typing. |
| Dependency installation fails | In Ubuntu run `sudo apt-get update` and `sudo apt-get install -y python3 build-essential`, then restart. |
| Browser did not open | Manually open the exact localhost URL printed by the launcher. |
| Default port occupied | Use the printed fallback URL or `bash start.sh --port 8090`. |
| Backend disconnected | Keep the launcher terminal open and check it for errors. Restart the launcher if needed. |
| Peer offline | Use Restart processes in Process inspector. |
| Windows browser cannot reach WSL | Use WSL 2 with localhost forwarding enabled, run `wsl --shutdown` and start again. |
| Ubuntu file opens in editor | Run `bash START_UBUNTU.sh` in a terminal from the project folder. |
| Setup needs internet | Only the first missing-prerequisite installation needs internet; normal operation does not. |

Prepare and test the Ubuntu/WSL environment before the live demonstration.
