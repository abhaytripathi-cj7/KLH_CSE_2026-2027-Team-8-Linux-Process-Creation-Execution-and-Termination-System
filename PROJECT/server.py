#!/usr/bin/env python3
"""Local web interface for the real C/Linux process demonstration (stdlib only)."""
import atexit
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = Path(__file__).resolve().parent
LOCK = threading.RLock()
PROC = None
DATA = {}


def reset():
    DATA.clear()
    DATA.update(stage="idle", parent=None, child=None, snapshots={}, events=[],
                logs=[], outcome=None, exit_code=None, signal=None, mode=None, amount=None)


reset()


def event(title, detail):
    DATA["events"].append(dict(title=title, detail=detail, time=time.strftime("%H:%M:%S")))


def live(pid):
    try:
        fields = {}
        for line in Path(f"/proc/{pid}/status").read_text().splitlines():
            key, _, value = line.partition(":")
            if key in ("Name", "State", "Pid", "PPid", "Uid", "VmRSS", "Threads"):
                fields[key] = value.strip()
        fields.update(PGID=str(os.getpgid(pid)), SID=str(os.getsid(pid)),
                      nice=str(os.getpriority(os.PRIO_PROCESS, pid)))
        return fields
    except (OSError, ProcessLookupError):
        return None


def collect(proc, context=None):
    DATA = globals()['DATA'] if context is None else context.data
    LOCK = globals()['LOCK'] if context is None else context.lock
    event = globals()['event'] if context is None else context.event
    section = None
    for raw in proc.stdout:
        line = raw.rstrip()
        with LOCK:
            DATA["logs"].append(line)
            if "==========" in line:
                section = "parent" if "PARENT:" in line else "child"
                DATA["snapshots"][section] = {}
                if "CREATION:" in line:
                    event("Process created", "fork() gives the payment child a new PID.")
                elif "EXECUTION:" in line:
                    event("Worker executing", "exec() replaces the program image; PID is preserved.")
                elif "TERMINATED:" in line:
                    event("Zombie observed", "waitid(WNOWAIT) retains the result until the parent reaps it.")
            match = re.match(r"(Name|State|Pid|PPid|Uid|VmRSS|Threads):\s*(.*)", line)
            if match and section:
                DATA["snapshots"][section][match[1]] = match[2]
                if match[1] == "Pid":
                    DATA[section] = int(match[2])
            if line.startswith("PGID=") and section:
                DATA["snapshots"][section].update(dict(re.findall(r"(PGID|SID|nice)=(-?\d+)", line)))
            if line.startswith("Relationship:"):
                DATA["stage"] = "waiting"
                event("Waiting for OTP", "The child is sleeping in read(); enter the demo code or decline.")
            if "WIFEXITED=true" in line:
                DATA["exit_code"] = int(line.split("WEXITSTATUS=")[1])
                DATA["outcome"] = "success" if DATA["exit_code"] == 0 else "failed"
            if "WIFSIGNALED=true" in line:
                DATA["signal"] = int(re.search(r"WTERMSIG=(\d+)", line)[1])
                DATA["outcome"] = "declined" if DATA["signal"] == 15 else "failed"
            if line.startswith("REAPED:"):
                event("Child reaped", "waitpid() collects the result and releases the process entry.")
    code = proc.wait()
    proc.stdout.close()
    if proc.stdin and not proc.stdin.closed:
        proc.stdin.close()
    with LOCK:
        DATA["stage"] = "done"
        if DATA["outcome"] is None:
            DATA["outcome"] = "failed"
            event("Worker error", f"The payment application ended unexpectedly (code {code}).")
        else:
            event("Payment " + DATA["outcome"], "Result collected. You can start another payment.")


def cleanup():
    with LOCK:
        if PROC and PROC.poll() is None:
            # Close input first so the C parent terminates and reaps its child.
            try:
                PROC.stdin.close()
                PROC.wait(timeout=3)
            except (OSError, subprocess.TimeoutExpired):
                try:
                    os.killpg(PROC.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass


atexit.register(cleanup)


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass

    def send(self, code, data, content_type="application/json"):
        body = json.dumps(data).encode() if content_type == "application/json" else data
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/api/status":
            with LOCK:
                result = json.loads(json.dumps(DATA))
                result["server_pid"] = os.getpid()
                for role in ("parent", "child"):
                    current = live(DATA[role]) if DATA[role] and DATA["stage"] != "done" else None
                    result[role + "_live"] = current is not None
                    if current:
                        result["snapshots"][role] = current
            return self.send(200, result)
        assets = {"/": ("index.html", "text/html; charset=utf-8"),
                  "/config.js": ("config.js", "text/javascript; charset=utf-8"),
                  "/app.js": ("app.js", "text/javascript; charset=utf-8"),
                  "/style.css": ("style.css", "text/css; charset=utf-8")}
        if self.path not in assets:
            return self.send(404, {"error": "Not found"})
        filename, kind = assets[self.path]
        self.send(200, (ROOT / "dist" / filename).read_bytes(), kind)

    def do_POST(self):
        global PROC
        # Local-only actions; reject cross-origin browser requests and simple forms.
        origin = self.headers.get("Origin")
        if origin and origin not in ("http://localhost:8000", "http://127.0.0.1:8000"):
            return self.send(403, {"error": "Use the localhost website."})
        if self.headers.get("Content-Type", "").split(";")[0] != "application/json":
            return self.send(415, {"error": "JSON required"})
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 2048:
                raise ValueError("Invalid request size")
            body = json.loads(self.rfile.read(length))
            if not isinstance(body, dict):
                raise ValueError("Expected an object")
            with LOCK:
                if self.path == "/api/start":
                    if DATA["stage"] not in ("idle", "done"):
                        return self.send(409, {"error": "Complete the current payment first."})
                    mode = body.get("mode")
                    amount = body.get("amount")
                    if mode not in ("UPI", "Card", "Net banking"):
                        raise ValueError("Choose a valid payment mode.")
                    if type(amount) is not int or not 1 <= amount <= 1000000:
                        raise ValueError("Amount must be a whole number from 1 to 1,000,000.")
                    proc = subprocess.Popen([str(ROOT / "payment")], cwd=ROOT,
                                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                            stderr=subprocess.STDOUT, text=True, bufsize=1,
                                            start_new_session=True)
                    reset()
                    PROC = proc
                    DATA.update(stage="starting", parent=proc.pid, mode=mode, amount=amount)
                    event("Payment started", f"{mode} · {amount} demo units")
                    proc.stdin.write(f"{('UPI', 'Card', 'Net banking').index(mode)+1}\n{amount}\npay\n")
                    proc.stdin.flush()
                    threading.Thread(target=collect, args=(proc,), daemon=True).start()
                elif self.path in ("/api/otp", "/api/decline"):
                    if DATA["stage"] != "waiting":
                        return self.send(409, {"error": "The payment is not waiting for an OTP."})
                    decline = self.path == "/api/decline"
                    otp = "decline" if decline else body.get("otp", "")
                    if not decline and (not isinstance(otp, str) or not re.fullmatch(r"[0-9]{6}", otp)):
                        raise ValueError("Enter a six-digit OTP.")
                    PROC.stdin.write(otp + "\n")
                    PROC.stdin.flush()
                    DATA["stage"] = "processing"
                    event("Decline requested" if decline else "OTP submitted",
                          "Parent sends SIGTERM to the child." if decline else "write() wakes the child to validate the OTP.")
                else:
                    return self.send(404, {"error": "Not found"})
            self.send(200, {"ok": True})
        except (ValueError, TypeError, json.JSONDecodeError) as exc:
            self.send(400, {"error": str(exc)})
        except OSError:
            self.send(500, {"error": "The payment process could not be reached. Refresh and try again."})


if __name__ == "__main__":
    if not Path("/proc/self/status").exists():
        raise SystemExit("Run this server on Linux or Ubuntu in WSL.")
    subprocess.run(["make"], cwd=ROOT, check=True)
    server = ThreadingHTTPServer(("127.0.0.1", 8000), Handler)
    print("Payment Process Lab: http://localhost:8000", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        cleanup()
