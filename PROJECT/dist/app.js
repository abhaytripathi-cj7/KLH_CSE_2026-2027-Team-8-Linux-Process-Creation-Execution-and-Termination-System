const $ = id => document.getElementById(id);
let state = null, busy = false, lastLog = '', lastEvents = '', offline = false;
const backend = (window.PROCESSPAY_BACKEND || '').replace(/\/$/,'');
let session = sessionStorage.getItem('processpay-session'), sessionPromise = null;
if(backend){document.querySelector('.local-tag').textContent='LIVE LINUX';}
async function ensureSession(){
  if(!backend || session) return;
  if(!sessionPromise) sessionPromise = (async()=>{
    const r=await fetch(backend+'/api/session',{method:'POST',headers:{'Content-Type':'application/json'},body:'{}'});
    const d=await r.json();if(!r.ok)throw new Error(d.error||'Cannot create session.');
    session=d.token;sessionStorage.setItem('processpay-session',session);
  })().finally(()=>{sessionPromise=null;});
  await sessionPromise;
}
async function api(path, body) {
  await ensureSession();
  const headers=backend?{'X-Process-Session':session}:{};
  if(body!==undefined) headers['Content-Type']='application/json';
  const response = await fetch(backend+'/api/' + path, {method:body===undefined?'GET':'POST',headers,body:body===undefined?undefined:JSON.stringify(body)});
  const data = await response.json();
  if(response.status===401 && backend){session=null;sessionStorage.removeItem('processpay-session');}
  if (!response.ok) throw new Error(data.error || 'Request failed.');
  return data;
}
function text(id, value) { $(id).textContent = value ?? '—'; }
function render(data) {
  state = data;
  const active = !['idle','done'].includes(data.stage), waiting = data.stage === 'waiting';
  const p = data.snapshots.parent || {}, c = data.snapshots.child || {};
  if(active){
    $('amount').value = data.amount;
    for(const radio of document.querySelectorAll('input[name="mode"]')) radio.checked = radio.value === data.mode;
  }
  $('setup').disabled = active || busy || offline;
  $('pay-button').firstChild.textContent = data.stage === 'done' ? 'Start another payment ' : 'Pay & create process ';
  $('otp-area').hidden = !waiting && data.stage !== 'processing';
  $('verify').disabled = !waiting || busy || offline; $('decline').disabled = !waiting || busy || offline;
  $('otp').disabled = !waiting || busy || offline;
  text('parent-pid', 'PID ' + (data.parent ?? '—')); text('parent-ppid', 'PPID ' + (p.PPid ?? '—'));
  text('child-pid', 'PID ' + (data.child ?? '—')); text('child-ppid', 'PPID ' + (c.PPid ?? '—'));
  text('process-state', data.stage === 'done' ? 'Reaped' : (c.State || 'Not created'));
  text('exit-code', data.exit_code); text('signal', data.signal ? `${data.signal} · SIGTERM` : '—');
  text('uid', c.Uid?.split(/\s+/)[0]); text('pgid', c.PGID); text('sid', c.SID); text('threads', c.Threads); text('memory', c.VmRSS); text('nice', c.nice);
  text('snapshot-label', data.stage === 'done' ? 'Final snapshot · process ended' : data.child_live ? 'Live · refreshes every 0.5s' : 'Waiting for payment');
  const title = data.stage === 'done' ? ({success:'Successful',declined:'Declined',failed:'Failed'}[data.outcome]) : ({idle:'Ready',starting:'Creating',waiting:'Waiting for OTP',processing:'Processing'}[data.stage]);
  text('state-badge', title); $('state-badge').className = 'badge ' + (data.outcome || data.stage);
  text('explanation', data.stage === 'done' ? 'The child briefly entered zombie state (Z). The parent called waitpid() to collect its result. Its PID is shown as a historical record; the child no longer exists.' : waiting ? 'State S means interruptible sleep. The child is blocked in read() on a pipe. Submitting an OTP wakes it; declining sends SIGTERM.' : data.stage === 'idle' ? 'Choose a payment mode and press Pay. A new child process will appear here with its own PID.' : 'The payment application and worker are real Linux processes. exec() changes the program image while keeping the same PID.');
  for (const el of document.querySelectorAll('[data-step]')) {
    const key = el.dataset.step;
    el.classList.toggle('active', key === 'create' ? !!data.parent : key === 'execute' ? !!data.child : key === 'wait' ? waiting : ['end','reap'].includes(key) && data.stage === 'done');
  }
  $('result').hidden = data.stage !== 'done';
  if (data.stage === 'done') {
    $('result').className = 'result' + (data.outcome === 'success' ? '' : ' bad');
    $('result').replaceChildren();
    const strong = document.createElement('strong'); strong.textContent = data.outcome === 'success' ? 'Payment successful' : data.outcome === 'declined' ? 'Payment declined' : 'Payment failed';
    const detail = document.createElement('p'); detail.textContent = data.signal ? `Child ${data.child} terminated by signal ${data.signal}. The parent collected the result.` : `Child ${data.child} exited normally with code ${data.exit_code}. ${data.exit_code === 1 ? 'The OTP did not match.' : data.exit_code === 0 ? 'Payment completed.' : 'Check the execution trace.'}`;
    $('result').append(strong, detail);
  }
  const events = JSON.stringify(data.events);
  if (events !== lastEvents && data.events.length) {
    $('events').replaceChildren();
    for (const item of data.events) {
      const row=document.createElement('div'); row.className='event';
      for (const [tag,value] of [['time',item.time],['strong',item.title],['p',item.detail]]) { const node=document.createElement(tag); node.textContent=value; row.append(node); }
      $('events').append(row);
    }
    lastEvents = events;
  }
  text('event-count', `${data.events.length} EVENTS`);
  const logs = data.logs.join('\n'); if (logs !== lastLog) {text('terminal',logs || 'Waiting for a process…');lastLog=logs;}
}
async function refresh() {
  try { const data = await api('status'); offline = false; text('connection','Linux connected'); $('connection').style.color=''; render(data); }
  catch { offline = true; text('connection','Server disconnected'); $('connection').style.color='#ab3a48'; $('setup').disabled=true; if(state) render(state); }
}
async function action(path, body) {
  if (busy) return;
  busy = true; $('error').hidden = true; if(state) render(state);
  try {await api(path,body); if(path==='start') $('otp').value=''; await refresh();}
  catch (err) {text('error',err.message);$('error').hidden=false;throw err;}
  finally {busy=false;if(state) render(state);}
}
$('payment-form').addEventListener('submit', e => {e.preventDefault();action('start',{mode:document.querySelector('input[name="mode"]:checked').value,amount:Number($('amount').value)}).catch(()=>{});});
$('otp-form').addEventListener('submit', e => {e.preventDefault();action('otp',{otp:$('otp').value}).catch(()=>{});});
$('decline').addEventListener('click',()=>action('decline',{}).catch(()=>{}));
async function poll(){await refresh();setTimeout(poll,500);} poll();
if(document.modelContext?.registerTool){
  const lifecycle=new AbortController();
  const tools=[
    {name:'read_payment_process',description:'Read the current payment and real Linux process details.',inputSchema:{type:'object',properties:{},additionalProperties:false},annotations:{readOnlyHint:true},execute:async()=>{await refresh();return state;}},
    {name:'start_demo_payment',description:'Create a real Linux payment child for a simulated payment.',inputSchema:{type:'object',properties:{mode:{type:'string',enum:['UPI','Card','Net banking']},amount:{type:'integer',minimum:1,maximum:1000000}},required:['mode','amount'],additionalProperties:false},execute:async input=>{await action('start',input);return {stage:state.stage,parent:state.parent};}},
    {name:'submit_demo_otp',description:'Submit the six-digit OTP to complete the current simulated payment.',inputSchema:{type:'object',properties:{otp:{type:'string',pattern:'^[0-9]{6}$'}},required:['otp'],additionalProperties:false},execute:async input=>{await action('otp',input);return {stage:state.stage};}},
    {name:'decline_demo_payment',description:'Decline the current simulated payment and terminate its Linux child with SIGTERM.',inputSchema:{type:'object',properties:{},additionalProperties:false},execute:async()=>{await action('decline',{});return {stage:state.stage};}}
  ];
  for(const tool of tools){try{Promise.resolve(document.modelContext.registerTool(tool,{signal:lifecycle.signal})).catch(()=>{});}catch{}}
  addEventListener('pagehide',()=>lifecycle.abort(),{once:true});
}
