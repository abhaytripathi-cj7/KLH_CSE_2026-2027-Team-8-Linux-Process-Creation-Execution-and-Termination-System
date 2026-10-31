"""Integration checks against real C peers and Linux named pipes."""
import concurrent.futures
import importlib.util
import json
import os
from pathlib import Path
import signal
import stat
import sys
import threading
import time
import unittest
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'backend'))

class IntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.find_spec('server')
        if spec is None:
            raise AssertionError('Linux IPC server implementation is missing')
        import server
        cls.engine = server.Engine()
        cls.http = server.make_server(cls.engine, port=0)
        cls.thread = threading.Thread(target=cls.http.serve_forever, daemon=True)
        cls.thread.start()
        cls.base = 'http://127.0.0.1:' + str(cls.http.server_port)

    @classmethod
    def tearDownClass(cls):
        runtime = cls.engine.runtime
        peers = list(cls.engine.peers.values())
        cls.http.shutdown()
        cls.http.server_close()
        cls.engine.close()
        assert not runtime.exists(), 'FIFO runtime directory leaked'
        for peer in peers:
            assert peer.poll() is not None, 'Child process leaked'

    def request(self, path, data=None, token=True):
        headers = {}
        if data is not None:
            headers['Content-Type'] = 'application/json'
            if token:
                headers['X-PipeTalk-Token'] = self.engine.token
        req = urllib.request.Request(self.base + path, data=None if data is None else json.dumps(data).encode(), headers=headers)
        with urllib.request.urlopen(req, timeout=5) as response:
            return json.loads(response.read())

    def delivered(self, message_id):
        deadline = time.monotonic() + 4
        while time.monotonic() < deadline:
            for message in self.request('/api/state')['messages']:
                if message['id'] == message_id and message['status'] == 'delivered':
                    return message
            time.sleep(.02)
        self.fail('C receiving process did not confirm delivery')

    def test_01_real_processes_and_fifos(self):
        state = self.request('/api/state')
        self.assertEqual(len(state['processes']), 2)
        self.assertNotEqual(state['processes'][0]['pid'], state['processes'][1]['pid'])
        for peer in state['processes']:
            self.assertTrue(peer['alive'])
            self.assertIn('pipetalk_peer', (self.engine.proc_path(peer['pid']) / 'comm').read_text())
            self.assertGreater(peer['rss_kb'], 0)
        for fifo in state['fifos']:
            self.assertTrue(stat.S_ISFIFO(os.stat(fifo['path']).st_mode))

    def test_02_bidirectional_transport(self):
        for sender in ['A', 'B']:
            sent = self.request('/api/send', {'sender': sender, 'text': 'Hello from ' + sender})
            received = self.delivered(sent['id'])
            self.assertEqual(received['text'], 'Hello from ' + sender)
            self.assertEqual(received['receiver'], 'B' if sender == 'A' else 'A')
            self.assertIsInstance(received['receiver_pid'], int)
            self.assertGreaterEqual(received['latency_ms'], 0)

    def test_03_unicode_multiline_and_html_are_preserved(self):
        text = 'Hello 👋 తెలుగు 日本語\nsecond line "quoted" \\ <script>alert(1)</script>'
        sent = self.request('/api/send', {'sender': 'A', 'text': text})
        self.assertEqual(self.delivered(sent['id'])['text'], text)

    def test_04_validation_and_csrf(self):
        for data in [{'sender': 'C', 'text': 'x'}, {'sender': 'A', 'text': '  '}, {'sender': 'A', 'text': 'x' * 1601}, {'sender': 'A', 'text': 42}, []]:
            with self.assertRaises(urllib.error.HTTPError) as ctx:
                self.request('/api/send', data)
            self.assertEqual(ctx.exception.code, 400)
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self.request('/api/send', {'sender': 'A', 'text': 'unauthorized'}, token=False)
        self.assertEqual(ctx.exception.code, 403)

    def test_05_simultaneous_messages(self):
        def send(i):
            return self.request('/api/send', {'sender': 'A' if i % 2 else 'B', 'text': 'Concurrent ' + str(i)})['id']
        with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
            ids = list(pool.map(send, range(20)))
        self.assertEqual(len(set(ids)), 20)
        for mid in ids:
            self.delivered(mid)

    def test_06_static_and_transcript(self):
        with urllib.request.urlopen(self.base + '/') as response:
            self.assertIn(b'PipeTalk', response.read())
        sent = self.request('/api/send', {'sender': 'A', 'text': 'Export verification'})
        self.delivered(sent['id'])
        transcript = self.request('/api/export')
        self.assertTrue(any(m['id'] == sent['id'] and m['text'] == 'Export verification' for m in transcript['messages']))

    def test_07_dead_peer_is_reported_and_can_restart(self):
        os.kill(self.engine.peers['B'].pid, signal.SIGTERM)
        self.engine.peers['B'].wait(timeout=2)
        state = self.request('/api/state')
        self.assertFalse(next(p for p in state['processes'] if p['name'] == 'B')['alive'])
        with self.assertRaises(urllib.error.HTTPError) as ctx:
            self.request('/api/send', {'sender': 'A', 'text': 'Must not fake delivery'})
        self.assertEqual(ctx.exception.code, 503)
        self.request('/api/restart', {})
        sent = self.request('/api/send', {'sender': 'B', 'text': 'Recovered'})
        self.delivered(sent['id'])

if __name__ == '__main__':
    unittest.main(verbosity=2)
