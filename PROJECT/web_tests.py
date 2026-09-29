"""Integration checks for a running local website; uses only synthetic payments."""
import json
from pathlib import Path
import time
from urllib.request import Request, urlopen
from urllib.error import HTTPError

BASE = 'http://127.0.0.1:8000'


def request(path, data=None):
    req = Request(BASE + '/api/' + path,
                  data=None if data is None else json.dumps(data).encode(),
                  headers={'Content-Type': 'application/json'})
    with urlopen(req, timeout=5) as response:
        return json.load(response)


def wait_for(stage):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        state = request('status')
        if state['stage'] == stage:
            return state
        time.sleep(.05)
    raise AssertionError(state)


def rejected(path, payload, status):
    try:
        request(path, payload)
    except HTTPError as error:
        assert error.code == status, error
    else:
        raise AssertionError('Request should have been rejected')


assert request('status')['stage'] in ('idle', 'done'), 'Finish the current payment first.'
rejected('start', {'mode': 'UPI', 'amount': 0}, 400)
for mode, action, body, outcome, code, sig in [
    ('UPI', 'otp', {'otp': '123456'}, 'success', 0, None),
    ('Card', 'otp', {'otp': '000000'}, 'failed', 1, None),
    ('Net banking', 'decline', {}, 'declined', None, 15),
]:
    request('start', {'mode': mode, 'amount': 500})
    state = wait_for('waiting')
    assert int(state['snapshots']['child']['PPid']) == state['parent']
    assert state['snapshots']['child']['State'].startswith('S')
    rejected('start', {'mode': 'UPI', 'amount': 500}, 409)
    rejected('otp', {'otp': '123'}, 400)
    request(action, body)
    state = wait_for('done')
    assert (state['outcome'], state['exit_code'], state['signal']) == (outcome, code, sig)
    assert not Path(f"/proc/{state['child']}").exists(), 'Child was not reaped'
    assert any(item['title'] == 'Zombie observed' for item in state['events'])
    rejected('decline', {}, 409)
    print(f'PASS: {mode}, {outcome}, real PIDs, sleeping, zombie, reaping, API validation')
print('All localhost website integration checks passed.')
