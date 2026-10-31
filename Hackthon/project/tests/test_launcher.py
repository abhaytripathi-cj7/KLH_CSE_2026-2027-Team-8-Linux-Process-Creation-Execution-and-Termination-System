"""Exercise extraction paths, busy-port fallback and full launcher shutdown."""
import json
import os
from pathlib import Path
import queue
import re
import shutil
import socket
import subprocess
import tempfile
import threading
import time
import unittest
import urllib.request

ROOT = Path(__file__).resolve().parents[1]

class LauncherTests(unittest.TestCase):
    def test_launcher_handles_spaces_busy_port_and_cleanup(self):
        with tempfile.TemporaryDirectory(prefix='pipetalk-test-') as tmp, socket.socket() as occupied:
            folder = Path(tmp) / 'Project with spaces'
            shutil.copytree(ROOT, folder, ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
            occupied.bind(('127.0.0.1', 0))
            occupied.listen()
            busy_port = occupied.getsockname()[1]
            process = subprocess.Popen(['bash', str(folder / 'START_UBUNTU.sh'), '--port', str(busy_port)],
                cwd=tmp, env={**os.environ, 'PIPETALK_NO_BROWSER': '1'},
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            lines = queue.Queue()
            def reader():
                for line in process.stdout:
                    lines.put(line)
            threading.Thread(target=reader, daemon=True).start()
            output = ''
            runtime = None
            try:
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    try:
                        output += lines.get(timeout=.2)
                    except queue.Empty:
                        if process.poll() is not None:
                            self.fail('Launcher exited early: ' + output)
                        continue
                    if 'Press Ctrl+C' in output:
                        break
                match = re.search(r'Dashboard: (http://localhost:(\d+))', output)
                self.assertIsNotNone(match, output)
                self.assertNotEqual(int(match[2]), busy_port)
                url = match[1].replace('localhost', '127.0.0.1')
                with urllib.request.urlopen(url + '/api/state', timeout=3) as response:
                    state = json.load(response)
                runtime = Path(state['runtime'])
                self.assertTrue(runtime.exists())
                self.assertTrue(all(peer['alive'] for peer in state['processes']))
                self.assertEqual(len(state['fifos']), 2)
            finally:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
                process.stdout.close()
            self.assertEqual(process.returncode, 0, output)
            self.assertIsNotNone(runtime)
            self.assertFalse(runtime.exists(), 'Launcher shutdown left FIFO directory behind')

if __name__ == '__main__':
    unittest.main(verbosity=2)
