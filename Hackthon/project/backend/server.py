#!/usr/bin/env python3
"""Local web bridge supervising two genuine Linux FIFO chat processes."""
import argparse
from collections import deque
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import secrets
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
from urllib.parse import urlsplit
import uuid
import webbrowser

ROOT = Path(__file__).resolve().parents[1]
MAX_TEXT_BYTES = 1600

def utc_now():
    return datetime.now(timezone.utc).isoformat(timespec='milliseconds')

class Unavailable(Exception):
    pass

class Engine:
    def __init__(self):
        if not sys.platform.startswith('linux'):
            raise RuntimeError('Run this backend inside Ubuntu or Ubuntu on WSL.')
        self.runtime = Path(tempfile.mkdtemp(prefix='pipetalk-'))
        self.lock = threading.RLock()
        self.lifecycle = threading.Lock()
        self.token = secrets.token_urlsafe(32)
        self.peers = {}
        self.ready = {}
        self.generation = 0
        self.proc_cache = {}
        self.events = deque(maxlen=200)
        self.messages = deque(maxlen=500)
        self.sequence = 0
        self.started = time.monotonic()
        self.total_sent = 0
        self.total_delivered = 0
        self.latency_total = 0.0
        self.binary = self.runtime / 'pipetalk_peer'
        self.fifos = {'A_to_B': self.runtime / 'a_to_b.fifo', 'B_to_A': self.runtime / 'b_to_a.fifo'}
        try:
            compiler = shutil.which('gcc')
            if not compiler:
                raise RuntimeError('GCC is missing. Run: sudo apt install build-essential')
            subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                            str(ROOT / 'backend/peer.c'), '-o', str(self.binary)], check=True)
            self.restart()
        except BaseException:
            self.close()
            raise

    def event(self, kind, detail, **extra):
        self.sequence += 1
        self.events.append({'seq': self.sequence, 'time': utc_now(), 'kind': kind, 'detail': detail, **extra})

    def _reader(self, name, process, generation):
        try:
            for raw in process.stdout:
                item = json.loads(raw)
                with self.lock:
                    if generation != self.generation:
                        return
                    kind = item['type']
                    if kind == 'ready':
                        self.event('ready', f'Process {name} opened both FIFO descriptors', peer=name, pid=process.pid)
                        self.ready[name].set()
                        continue
                    payload = item.get('payload', {})
                    message = next((m for m in self.messages if m['id'] == payload.get('id')), None)
                    if message is None:
                        continue
                    if kind == 'received' and name == message['receiver']:
                        # Accept delivery only from the destination process after
                        # matching the bytes it actually read from the FIFO.
                        if payload.get('text') != message['text'] or payload.get('sender') != message['sender']:
                            self.event('error', 'Received frame did not match the submitted message')
                            continue
                        if message['status'] != 'delivered':
                            latency = round((time.monotonic() - message['_start']) * 1000, 3)
                            message.update(status='delivered', receiver_pid=process.pid, received_at=utc_now(), latency_ms=latency)
                            self.total_delivered += 1
                            self.latency_total += latency
                        self.event('received', f'Process {name} read() from FIFO', peer=name, pid=process.pid, message_id=message['id'])
                    elif kind == 'sent':
                        self.event('sent', f'Process {name} write() to FIFO', peer=name, pid=process.pid, message_id=message['id'])
                    elif kind == 'failed':
                        message['status'] = 'failed'
                        self.event('error', f'Process {name} could not write to FIFO', peer=name, message_id=message['id'])
        except (ValueError, OSError, KeyError) as exc:
            with self.lock:
                if generation == self.generation:
                    self.event('error', f'Process {name} event stream stopped: {exc}')
        finally:
            with self.lock:
                if generation == self.generation:
                    self.event('offline', f'Process {name} exited', peer=name, pid=process.pid)

    def _stop(self):
        with self.lock:
            self.generation += 1
            peers = list(self.peers.values())
            self.peers = {}
            for message in self.messages:
                if message['status'] == 'pending':
                    message['status'] = 'failed'
        for process in peers:
            if process.poll() is None:
                process.terminate()
        for process in peers:
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=2)
            for stream in (process.stdin, process.stdout):
                if stream:
                    stream.close()

    def restart(self):
        with self.lifecycle:
            self._stop()
            with self.lock:
                for path in self.fifos.values():
                    path.unlink(missing_ok=True)
                    os.mkfifo(path, 0o600)
                self.event('system', 'Created two Linux named pipes using mkfifo()')
                for name in ('A', 'B'):
                    incoming = self.fifos['B_to_A' if name == 'A' else 'A_to_B']
                    outgoing = self.fifos['A_to_B' if name == 'A' else 'B_to_A']
                    process = subprocess.Popen([str(self.binary), name, str(incoming), str(outgoing)],
                                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=None, bufsize=0)
                    os.set_blocking(process.stdin.fileno(), False)
                    self.peers[name] = process
                    self.ready[name] = threading.Event()
                    threading.Thread(target=self._reader, args=(name, process, self.generation), daemon=True).start()
            for name in ('A', 'B'):
                if not self.ready[name].wait(3) or self.peers[name].poll() is not None:
                    raise Unavailable(f'Process {name} did not start. Check the terminal.')

    def send(self, data):
        if not isinstance(data, dict):
            raise ValueError('A JSON object is required.')
        sender, text = data.get('sender'), data.get('text')
        if sender not in ('A', 'B'):
            raise ValueError('Choose Process A or Process B.')
        if not isinstance(text, str) or not text.strip():
            raise ValueError('Enter a non-empty message.')
        if len(text.encode('utf-8')) > MAX_TEXT_BYTES:
            raise ValueError('Message exceeds 1600 UTF-8 bytes.')
        receiver = 'B' if sender == 'A' else 'A'
        with self.lock:
            if any(self.peers.get(n) is None or self.peers[n].poll() is not None for n in ('A', 'B')):
                raise Unavailable('Both Linux processes must be running. Use Restart processes.')
            # Keep all outstanding frames until transport confirms or fails.
            if sum(m['status'] == 'pending' for m in self.messages) >= 100:
                raise Unavailable('Too many pending messages. Wait for delivery or restart processes.')
            mid = uuid.uuid4().hex
            message = {'id': mid, 'sender': sender, 'receiver': receiver, 'text': text, 'sent_at': utc_now(),
                       'sender_pid': self.peers[sender].pid, 'receiver_pid': None, 'status': 'pending',
                       'latency_ms': None, '_start': time.monotonic()}
            frame = (json.dumps({'id': mid, 'sender': sender, 'text': text}, ensure_ascii=False, separators=(',', ':')) + '\n').encode('utf-8')
            if len(frame) > 4096:
                raise ValueError('Encoded message exceeds the atomic pipe frame size. Reduce control characters.')
            self.messages.append(message)
            try:
                written = os.write(self.peers[sender].stdin.fileno(), frame)
                if written != len(frame):
                    raise OSError('Incomplete control frame')
            except (OSError, ValueError) as exc:
                message['status'] = 'failed'
                raise Unavailable('Process input is unavailable or busy. Try again.') from exc
            self.total_sent += 1
            self.event('queued', f'Message queued for Process {sender}', peer=sender, message_id=mid)
            return {'id': mid, 'status': 'pending'}

    @staticmethod
    def public(message):
        return {k: v for k, v in message.items() if not k.startswith('_')}

    def proc_path(self, pid):
        """Resolve /proc even in containers with an outer-namespace proc mount."""
        cached = self.proc_cache.get(pid)
        if cached is not None and cached.exists():
            return cached
        self_status = Path('/proc/self/status').read_text()
        proc_pid = next(int(line.split()[1]) for line in self_status.splitlines() if line.startswith('Pid:'))
        if proc_pid == os.getpid():
            return Path('/proc') / str(pid)
        namespace = os.readlink('/proc/self/ns/pid')
        for entry in Path('/proc').iterdir():
            if not entry.name.isdigit():
                continue
            try:
                if os.readlink(entry / 'ns/pid') != namespace:
                    continue
                for line in (entry / 'status').read_text().splitlines():
                    if line.startswith('NSpid:') and int(line.split()[-1]) == pid:
                        self.proc_cache[pid] = entry
                        return entry
            except (OSError, ValueError):
                continue
        return Path('/proc') / str(pid)

    def snapshot(self):
        with self.lock:
            processes = []
            for name, process in self.peers.items():
                alive = process.poll() is None
                info = {'name': name, 'pid': process.pid, 'alive': alive, 'state': 'Exited', 'rss_kb': 0, 'fd_count': 0}
                if alive:
                    try:
                        proc = self.proc_path(process.pid)
                        fields = dict(line.split(':', 1) for line in (proc / 'status').read_text().splitlines() if ':' in line)
                        info.update(state=fields.get('State', '').strip(), rss_kb=int(fields.get('VmRSS', '0 kB').split()[0]),
                                    fd_count=len(list((proc / 'fd').iterdir())))
                    except (OSError, ValueError):
                        pass
                processes.append(info)
            # A pending frame is explicitly failed if its destination exits.
            alive_by_name = {p['name']: p['alive'] for p in processes}
            for message in self.messages:
                if message['status'] == 'pending' and (not alive_by_name.get(message['receiver']) or time.monotonic() - message['_start'] > 5):
                    message['status'] = 'failed'
            return {'token': self.token, 'runtime': str(self.runtime), 'platform': 'Linux', 'server_pid': os.getpid(),
                    'uptime_seconds': int(time.monotonic() - self.started), 'processes': processes,
                    'fifos': [{'name': k, 'path': str(v), 'exists': v.exists()} for k, v in self.fifos.items()],
                    'messages': [self.public(m) for m in self.messages], 'events': list(self.events),
                    'stats': {'sent': self.total_sent, 'delivered': self.total_delivered,
                              'average_latency_ms': round(self.latency_total / self.total_delivered, 3) if self.total_delivered else None},
                    'limits': {'message_bytes': MAX_TEXT_BYTES, 'messages_retained': 500, 'events_retained': 200}}

    def close(self):
        with self.lifecycle:
            self._stop()
            shutil.rmtree(self.runtime, ignore_errors=True)

def make_server(engine, port=8080):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def reply(self, code, content, mime='application/json; charset=utf-8', download=False):
            body = json.dumps(content, ensure_ascii=False).encode('utf-8') if mime.startswith('application/json') else content
            self.send_response(code)
            self.send_header('Content-Type', mime)
            self.send_header('Content-Length', str(len(body)))
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            self.send_header('Content-Security-Policy', "default-src 'self'; style-src 'self'; script-src 'self'; img-src 'self' data:; connect-src 'self'; frame-ancestors 'none'")
            if download:
                self.send_header('Content-Disposition', 'attachment; filename="PipeTalk-Session.json"')
            self.end_headers()
            try:
                self.wfile.write(body)
            except (BrokenPipeError, ConnectionResetError):
                pass

        def host_allowed(self):
            return self.headers.get('Host', '') in (f'localhost:{self.server.server_port}', f'127.0.0.1:{self.server.server_port}')

        def do_GET(self):
            if not self.host_allowed():
                return self.reply(403, {'error': 'Use the localhost URL printed in the terminal.'})
            path = urlsplit(self.path).path
            if path == '/api/state':
                return self.reply(200, engine.snapshot())
            if path == '/api/export':
                state = engine.snapshot()
                return self.reply(200, {'project': 'PipeTalk', 'exported_at': utc_now(), 'messages': state['messages'], 'events': state['events']}, download=True)
            files = {'/': ('index.html', 'text/html; charset=utf-8'), '/style.css': ('style.css', 'text/css; charset=utf-8'), '/app.js': ('app.js', 'text/javascript; charset=utf-8')}
            if path in files:
                filename, mime = files[path]
                return self.reply(200, (ROOT / 'frontend' / filename).read_bytes(), mime)
            return self.reply(404, {'error': 'Not found.'})

        def do_POST(self):
            if not self.host_allowed() or self.headers.get('X-PipeTalk-Token') != engine.token:
                return self.reply(403, {'error': 'Reload the dashboard before sending commands.'})
            origin = self.headers.get('Origin')
            if origin and origin not in (f'http://localhost:{self.server.server_port}', f'http://127.0.0.1:{self.server.server_port}'):
                return self.reply(403, {'error': 'Cross-origin commands are not allowed.'})
            try:
                length = int(self.headers.get('Content-Length', '0'))
                if length < 1 or length > 16384:
                    raise ValueError('Request body must be between 1 and 16384 bytes.')
                self.connection.settimeout(5)
                data = json.loads(self.rfile.read(length))
                path = urlsplit(self.path).path
                if path == '/api/send':
                    return self.reply(202, engine.send(data))
                if path == '/api/restart':
                    engine.restart()
                    return self.reply(200, {'ok': True})
                return self.reply(404, {'error': 'Not found.'})
            except (ValueError, UnicodeError):
                return self.reply(400, {'error': 'Invalid message: choose A or B and enter text of at most 1600 UTF-8 bytes.'})
            except Unavailable as exc:
                return self.reply(503, {'error': str(exc)})
            except (OSError, subprocess.SubprocessError) as exc:
                return self.reply(503, {'error': f'Linux backend error: {exc}'})
    return ThreadingHTTPServer(('127.0.0.1', port), Handler)

def main():
    parser = argparse.ArgumentParser(description='PipeTalk Linux IPC web server')
    parser.add_argument('--port', type=int, default=8080)
    parser.add_argument('--open-browser', action='store_true')
    args = parser.parse_args()
    engine = None
    http = None
    try:
        engine = Engine()
        # A busy default port is handled without stopping another application.
        for port in range(args.port, min(args.port + 20, 65536)):
            try:
                http = make_server(engine, port)
                break
            except OSError as exc:
                if exc.errno != 98:
                    raise
        if http is None:
            raise RuntimeError('No available HTTP port. Choose another with --port.')
        url = f'http://localhost:{http.server_port}'
        print(f'\nPipeTalk | Linux IPC Chat\nDashboard: {url}\nRuntime:   {engine.runtime}\n', flush=True)
        for name, peer in engine.peers.items():
            print(f'Process {name}: PID {peer.pid}', flush=True)
        print('\nPress Ctrl+C to stop the server and clean up both processes and FIFOs.\n', flush=True)
        def interrupt(_sig, _frame):
            raise KeyboardInterrupt
        signal.signal(signal.SIGTERM, interrupt)
        if args.open_browser:
            def open_page():
                try:
                    if 'microsoft' in Path('/proc/sys/kernel/osrelease').read_text().lower() and shutil.which('powershell.exe'):
                        subprocess.Popen(['powershell.exe', '-NoProfile', '-Command', f"Start-Process '{url}'"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    else:
                        webbrowser.open(url)
                except OSError:
                    pass
            threading.Thread(target=open_page, daemon=True).start()
        http.serve_forever(poll_interval=.25)
    except KeyboardInterrupt:
        print('\nStopping PipeTalk...', flush=True)
    except (RuntimeError, OSError, subprocess.SubprocessError) as exc:
        print(f'Cannot start PipeTalk: {exc}', file=sys.stderr)
        return 1
    finally:
        if http:
            http.server_close()
        if engine:
            engine.close()
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
