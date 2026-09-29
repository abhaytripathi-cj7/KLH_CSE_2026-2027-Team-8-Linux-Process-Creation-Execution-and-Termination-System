"""Session-isolated Linux backend for a Netlify frontend, behind an HTTPS proxy."""
import copy
import json
import os
import secrets
import signal
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import server as core

SESSIONS = {}
LOCK = threading.RLock()
ORIGINS = set(filter(None, os.environ.get('ALLOWED_ORIGINS', '').split(',')))
TIMEOUT = int(os.environ.get('PAYMENT_TIMEOUT', '300'))
STOP = threading.Event()


class Session:
    def __init__(self):
        self.lock = threading.RLock()
        self.data = copy.deepcopy(core.DATA)
        self.proc = None
        self.updated = time.monotonic()
        self.started = 0

    def event(self, title, detail):
        self.data['events'].append(dict(title=title, detail=detail, time=time.strftime('%H:%M:%S')))

    def stop(self):
        if self.proc and self.proc.poll() is None:
            try:
                self.proc.stdin.close()
                self.proc.wait(timeout=3)
            except (OSError, ValueError, subprocess.TimeoutExpired):
                try:
                    os.killpg(self.proc.pid, signal.SIGTERM)
                    self.proc.wait(timeout=3)
                except (ProcessLookupError, subprocess.TimeoutExpired):
                    pass


def sweeper():
    while not STOP.wait(2):
        with LOCK:
            for token, session in list(SESSIONS.items()):
                with session.lock:
                    if session.proc and session.proc.poll() is None and time.monotonic()-session.started > TIMEOUT:
                        session.event('Payment timed out', 'OTP deadline reached. Closing input terminates and reaps the worker.')
                        session.stop()
                    if time.monotonic()-session.updated > 1800:
                        session.stop()
                        del SESSIONS[token]


class Handler(BaseHTTPRequestHandler):
    def setup(self):
        super().setup()
        self.connection.settimeout(10)

    def log_message(self, *args):
        pass

    def send(self, code, data):
        body = json.dumps(data).encode()
        self.send_response(code)
        origin = self.headers.get('Origin', '')
        if origin in ORIGINS:
            self.send_header('Access-Control-Allow-Origin', origin)
            self.send_header('Vary', 'Origin')
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.end_headers()
        self.wfile.write(body)

    def allowed(self):
        origin = self.headers.get('Origin')
        if origin and origin not in ORIGINS:
            self.send(403, {'error': 'This website origin is not allowed.'})
            return False
        return True

    def session(self):
        with LOCK:
            session = SESSIONS.get(self.headers.get('X-Process-Session', ''))
            if session:
                session.updated = time.monotonic()
        if not session:
            self.send(401, {'error': 'Session expired. Refresh to start a new session.'})
        return session

    def do_OPTIONS(self):
        if not self.allowed():
            return
        self.send_response(204)
        origin = self.headers.get('Origin', '')
        if origin in ORIGINS:
            self.send_header('Access-Control-Allow-Origin', origin)
            self.send_header('Vary', 'Origin')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, X-Process-Session')
        self.send_header('Access-Control-Max-Age', '600')
        self.end_headers()

    def do_GET(self):
        if self.path == '/health':
            return self.send(200, {'ok': True})
        if not self.allowed():
            return
        if self.path != '/api/status':
            return self.send(404, {'error': 'Not found'})
        session = self.session()
        if not session:
            return
        with session.lock:
            result = copy.deepcopy(session.data)
            for role in ('parent', 'child'):
                pid = result[role]
                current = core.live(pid) if pid and result['stage'] != 'done' else None
                result[role + '_live'] = current is not None
                if current:
                    result['snapshots'][role] = current
        self.send(200, result)

    def do_POST(self):
        if not self.allowed():
            return
        try:
            if self.headers.get('Content-Type', '').split(';')[0] != 'application/json':
                return self.send(415, {'error': 'JSON required'})
            size = int(self.headers.get('Content-Length', '0'))
            if not 0 < size <= 2048:
                raise ValueError('Invalid request size')
            body = json.loads(self.rfile.read(size))
            if not isinstance(body, dict):
                raise ValueError('Expected an object')
            if self.path == '/api/session':
                with LOCK:
                    if len(SESSIONS) >= 100:
                        return self.send(429, {'error': 'The lab is full. Please try later.'})
                    token = secrets.token_urlsafe(32)
                    SESSIONS[token] = Session()
                return self.send(201, {'token': token})
            session = self.session()
            if not session:
                return
            with LOCK, session.lock:
                data = session.data
                if self.path == '/api/start':
                    if data['stage'] not in ('idle', 'done'):
                        return self.send(409, {'error': 'Complete the current payment first.'})
                    if time.monotonic() - session.started < 2:
                        return self.send(429, {'error': 'Wait a moment before starting another payment.'})
                    if sum(s.proc is not None and s.proc.poll() is None for s in SESSIONS.values()) >= 8:
                        return self.send(429, {'error': 'All workers are busy. Try again shortly.'})
                    mode, amount = body.get('mode'), body.get('amount')
                    modes = ('UPI', 'Card', 'Net banking')
                    if mode not in modes or type(amount) is not int or not 1 <= amount <= 1000000:
                        raise ValueError('Choose a valid mode and amount.')
                    proc = subprocess.Popen([str(core.ROOT / 'payment')], cwd=core.ROOT,
                                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                            stderr=subprocess.STDOUT, text=True, bufsize=1,
                                            start_new_session=True)
                    session.data = copy.deepcopy(core.DATA)
                    data = session.data
                    data.update(stage='starting', parent=proc.pid, mode=mode, amount=amount)
                    session.proc, session.started = proc, time.monotonic()
                    session.event('Payment started', f'{mode} · {amount} demo units')
                    proc.stdin.write(f'{modes.index(mode)+1}\n{amount}\npay\n')
                    proc.stdin.flush()
                    threading.Thread(target=core.collect, args=(proc, session), daemon=True).start()
                elif self.path in ('/api/otp', '/api/decline'):
                    if data['stage'] != 'waiting':
                        return self.send(409, {'error': 'This payment is not waiting for an OTP.'})
                    decline = self.path.endswith('decline')
                    otp = 'decline' if decline else body.get('otp', '')
                    if not decline and (not isinstance(otp, str) or not core.re.fullmatch(r'[0-9]{6}', otp)):
                        raise ValueError('Enter a six-digit OTP.')
                    session.proc.stdin.write(otp+'\n')
                    session.proc.stdin.flush()
                    data['stage'] = 'processing'
                    session.event('Decline requested' if decline else 'OTP submitted',
                                  'Parent sends SIGTERM.' if decline else 'write() wakes the child.')
                else:
                    return self.send(404, {'error': 'Not found'})
            self.send(200, {'ok': True})
        except (ValueError, TypeError):
            self.send(400, {'error': 'Invalid input. Check the mode, amount or six-digit OTP.'})
        except OSError:
            self.send(503, {'error': 'Worker unavailable. Refresh and try again.'})


if __name__ == '__main__':
    if not ORIGINS:
        raise SystemExit('Set ALLOWED_ORIGINS to the exact HTTPS Netlify origin.')
    if not os.access(core.ROOT / 'payment', os.X_OK):
        raise SystemExit('Build the Linux worker first: make')
    http = ThreadingHTTPServer((os.environ.get('HOST', '127.0.0.1'), int(os.environ.get('PORT', '8001'))), Handler)
    threading.Thread(target=sweeper, daemon=True).start()
    def stop_server(*_):
        threading.Thread(target=http.shutdown, daemon=True).start()
    signal.signal(signal.SIGTERM, stop_server)
    signal.signal(signal.SIGINT, stop_server)
    print(f'Linux backend ready on {http.server_address}', flush=True)
    try:
        http.serve_forever()
    finally:
        STOP.set()
        http.server_close()
        with LOCK:
            for session in SESSIONS.values():
                with session.lock:
                    session.stop()
