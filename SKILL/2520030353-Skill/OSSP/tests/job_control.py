import os
import select
import signal
import sys
import time
from pathlib import Path


shell = Path(__file__).resolve().parents[1] / "ossp-shell"
pid, terminal = os.forkpty()
if pid == 0:
    os.execv(str(shell), [str(shell)])


def read_until(marker: bytes, timeout: float = 4.0) -> bytes:
    deadline = time.monotonic() + timeout
    output = bytearray()
    while time.monotonic() < deadline:
        ready, _, _ = select.select([terminal], [], [], min(0.1, deadline - time.monotonic()))
        if ready:
            chunk = os.read(terminal, 4096)
            output.extend(chunk)
            if marker in output:
                return bytes(output)
    raise AssertionError(f"timed out waiting for {marker!r}; output={output!r}")


try:
    read_until(b"ossp> ")
    os.write(terminal, b"sleep 10\n")
    time.sleep(0.2)
    os.write(terminal, b"\x03")
    read_until(b"ossp> ", 3)

    os.write(terminal, b"sleep 10\n")
    time.sleep(0.2)
    os.write(terminal, b"\x1a")
    stopped = read_until(b"ossp> ", 3)
    assert b"Stopped" in stopped, stopped

    os.write(terminal, b"jobs\n")
    jobs = read_until(b"ossp> ")
    assert b"Stopped" in jobs, jobs

    os.write(terminal, b"bg\n")
    running = read_until(b"ossp> ")
    assert b"Running" in running, running

    os.write(terminal, b"fg\n")
    time.sleep(0.2)
    os.write(terminal, b"\x03")
    read_until(b"ossp> ", 3)

    os.write(terminal, b"exit 0\n")
    os.waitpid(pid, 0)
    print("job control signal tests passed")
finally:
    try:
        os.kill(pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    os.close(terminal)
