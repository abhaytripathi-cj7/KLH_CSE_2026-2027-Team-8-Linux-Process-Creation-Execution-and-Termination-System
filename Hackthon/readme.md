# PipeTalk — Linux IPC Chat Application

**OSSP Hackathon · Team 8**

PipeTalk demonstrates two-way communication between independent Linux processes using named pipes (FIFOs). A browser dashboard presents two chat endpoints, while two C peer processes exchange message bytes through the Linux IPC channels. A Python HTTP bridge manages the processes and connects them to the interface.

## Repository contents

| Path | Contents |
|---|---|
| [project/](project/) | Complete extracted application source, launchers, and tests |
| [Project guide](project/README.md) | Detailed setup, architecture, demonstration, and troubleshooting |
| [Project report](PipeTalk_Team8_Project_Report.pdf) | PDF report |
| [Presentation](PipeTalk_Team8_Presentation.pptx) | PowerPoint slides |

## Features

- Bidirectional chat through two Linux named pipes.
- Live transport events and delivery confirmations.
- Process inspector showing PIDs, process state, memory, and FIFO paths.
- Peer restart and JSON session export.
- Ubuntu and Windows with Ubuntu on WSL launchers.

## Requirements

Ubuntu 22.04 or newer (native or WSL 2), Python 3.10+, GCC, and a modern browser. No Node.js, pip packages, database, or external service is required. The launcher can install missing Ubuntu prerequisites, which requires internet access and may request a sudo password.

## Quick start

Clone the repository:

```bash
git clone https://github.com/pranav269-commits/OSSP_Team-8_Hack.git
cd OSSP_Team-8_Hack/project
```

On Ubuntu:

```bash
bash START_UBUNTU.sh
```

On Windows, install and initialize Ubuntu on WSL 2, then open the `project` folder and double-click `START_WINDOWS.bat`.

Open the URL printed by the launcher, normally **http://localhost:8080**. Keep the terminal open while using the application; press **Ctrl+C** to stop. If port 8080 is busy, the application selects an available port through 8099.

## How it works

The Python bridge creates two FIFOs and starts separate C peer processes. Each peer writes outgoing messages to one FIFO and reads incoming messages from the other. The browser polls the bridge for chat and process updates. Delivery is confirmed when the destination peer reports receipt.

This is a local educational demonstration: the server binds to `127.0.0.1`, and session history remains in memory until the server stops. Export a session before stopping if you need to retain it.

## Tests

From the `project` directory:

```bash
python3 -m unittest discover -s tests -v
```

See the [complete project guide](project/README.md) for inspection commands, failure recovery, implementation details, and troubleshooting.
