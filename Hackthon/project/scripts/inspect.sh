#!/usr/bin/env bash
set -euo pipefail
PORT="${1:-8080}"
python3 - "$PORT" <<'PY'
import json
import sys
import urllib.request
port = int(sys.argv[1])
try:
    with urllib.request.urlopen(f'http://127.0.0.1:{port}/api/state', timeout=5) as response:
        state = json.load(response)
except OSError as exc:
    raise SystemExit(f'Cannot reach PipeTalk on port {port}. Start the project first. {exc}')
print('PipeTalk — live Linux backend')
print('Runtime:', state['runtime'])
print('Bridge PID:', state['server_pid'])
for peer in state['processes']:
    print(f"Process {peer['name']}: PID {peer['pid']} | {peer['state']} | RSS {peer['rss_kb']} KB | FDs {peer['fd_count']}")
print('\nRun these commands in Ubuntu:')
print('ps -p ' + ','.join(str(p['pid']) for p in state['processes']) + ' -o pid,ppid,stat,comm,args')
print(f"ls -l '{state['runtime']}'/*.fifo")
print('\nFIFO channels:')
for fifo in state['fifos']:
    print(f"  {fifo['name']}: {fifo['path']}")
print('\nMessages submitted:', state['stats']['sent'])
print('Messages delivered:', state['stats']['delivered'])
PY
