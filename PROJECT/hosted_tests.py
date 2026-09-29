"""Run against a disposable hosted backend on port 8011 with PAYMENT_TIMEOUT=8."""
import json
import time
from pathlib import Path
from urllib.request import Request, urlopen
from urllib.error import HTTPError

BASE='http://127.0.0.1:8011'

def call(path, token='', body=None, origin='http://localhost:8010', expected=200):
    headers={'Origin':origin,'X-Process-Session':token,'Content-Type':'application/json'}
    req=Request(BASE+'/api/'+path,headers=headers,data=None if body is None else json.dumps(body).encode())
    try:
        with urlopen(req,timeout=5) as r:
            assert r.status==expected
            assert r.headers['Access-Control-Allow-Origin']==origin
            return json.load(r)
    except HTTPError as e:
        assert e.code==expected,(e.code,expected)

def wait(token, stage, seconds=5):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        data=call('status',token)
        if data['stage']==stage:return data
        time.sleep(.1)
    raise AssertionError(data)

call('status',expected=401)
call('session',body={},origin='https://untrusted.example',expected=403)
a=call('session',body={},expected=201)['token']
b=call('session',body={},expected=201)['token']
call('start',a,{'mode':'UPI','amount':500})
call('start',b,{'mode':'Card','amount':700})
aa,bb=wait(a,'waiting'),wait(b,'waiting')
assert aa['child']!=bb['child']
assert int(aa['snapshots']['child']['PPid'])==aa['parent']
call('otp',a,{'otp':'123456'})
assert wait(a,'done')['outcome']=='success'
assert call('status',b)['stage']=='waiting'
call('decline',b,{})
assert wait(b,'done')['signal']==15
assert not Path(f"/proc/{aa['child']}").exists()
assert not Path(f"/proc/{bb['child']}").exists()
print('PASS: separate visitors, real PID/PPID, success, decline and reaping')
c=call('session',body={},expected=201)['token']
call('start',c,{'mode':'Net banking','amount':200})
wait(c,'waiting')
call('otp',c,{'otp':'000000'})
assert wait(c,'done')['exit_code']==1
print('PASS: incorrect OTP exits with code 1')
d=call('session',body={},expected=201)['token']
call('start',d,{'mode':'UPI','amount':100})
dd=wait(d,'waiting')
assert wait(d,'done',14)['signal']==15
assert not Path(f"/proc/{dd['child']}").exists()
print('PASS: deadline terminates and reaps abandoned payment')
print('PASS: unknown session and untrusted browser origin rejected')
