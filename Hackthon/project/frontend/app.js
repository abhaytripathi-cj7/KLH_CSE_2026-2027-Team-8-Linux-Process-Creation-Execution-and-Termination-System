'use strict';
const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => [...document.querySelectorAll(selector)];
const node = (tag, className, text) => {
  const el = document.createElement(tag);
  if (className) el.className = className;
  if (text !== undefined) el.textContent = text;
  return el;
};
let state = null;
let messageSignature = '';
let eventSequence = -1;
let connected = false;
let commands = '';
let failureCount = 0;
const sending = new Set();
const headings = {
  chat: ['Live chat', 'A conversation, at process level.', 'Send a message. Watch Linux carry it from one process to another.'],
  processes: ['Process inspector', 'Real processes. Visible connections.', 'Inspect the Linux processes and named pipes behind the conversation.'],
  guide: ['How it works', 'Follow the message.', 'A closer look at bidirectional inter-process communication on Linux.']
};

$$('.nav-button').forEach(button => button.addEventListener('click', () => {
  const view = button.dataset.view;
  $$('.nav-button').forEach(item => {
    item.classList.toggle('selected', item === button);
    if (item === button) item.setAttribute('aria-current', 'page');
    else item.removeAttribute('aria-current');
  });
  $$('.view').forEach(item => { item.hidden = item.id !== `view-${view}`; });
  const [breadcrumb, title, description] = headings[view];
  $('#breadcrumb-name').textContent = breadcrumb;
  $('#page-title').textContent = title;
  $('#page-description').textContent = description;
}));

function notify(message) {
  $('#notice').textContent = message;
  $('#notice').hidden = !message;
}

async function api(path, data) {
  const options = data === undefined ? {} : {
    method: 'POST',
    headers: {'Content-Type': 'application/json', 'X-PipeTalk-Token': state?.token || ''},
    body: JSON.stringify(data)
  };
  const response = await fetch(path, {...options, signal: AbortSignal.timeout(8000)});
  const result = await response.json();
  if (!response.ok) throw new Error(result.error || 'The request could not be completed.');
  return result;
}

function timeLabel(value, withSeconds = false) {
  return new Date(value).toLocaleTimeString([], {hour: '2-digit', minute: '2-digit', ...(withSeconds ? {second: '2-digit'} : {})});
}

function renderMessages(messages) {
  const signature = messages.map(m => `${m.id}:${m.status}`).join('|');
  if (signature === messageSignature) return;
  messageSignature = signature;
  for (const peer of ['A', 'B']) {
    const container = $(`#messages-${peer.toLowerCase()}`);
    const atBottom = container.scrollHeight - container.scrollTop - container.clientHeight < 65;
    const oldScroll = container.scrollTop;
    container.replaceChildren();
    for (const message of messages) {
      const outgoing = message.sender === peer;
      // A destination does not display an unreceived message as incoming.
      if (!outgoing && message.status !== 'delivered') continue;
      const wrapper = node('div', `message${outgoing ? ' outgoing' : ''}`);
      const bubble = node('div', 'bubble', message.text);
      const meta = node('div', 'message-meta');
      meta.append(node('span', '', timeLabel(message.sent_at)));
      if (outgoing) {
        meta.append(node('span', message.status, message.status === 'delivered' ? `✓✓ Delivered · ${message.latency_ms} ms` : message.status === 'failed' ? 'Failed' : 'Queued…'));
      } else {
        meta.append(node('span', '', `· from Process ${message.sender}`));
      }
      wrapper.append(bubble, meta);
      container.append(wrapper);
    }
    if (!container.children.length) {
      const empty = node('div', 'empty-state');
      empty.append(node('div', 'empty-icon', '⇄'), node('h4', '', 'Ready when you are.'), node('p', '', `Write as Process ${peer} to begin the conversation.`));
      container.append(empty);
    }
    container.scrollTop = atBottom ? container.scrollHeight : oldScroll;
  }
}

function renderEvents(events) {
  const last = events.at(-1)?.seq ?? 0;
  if (last === eventSequence) return;
  eventSequence = last;
  const container = $('#event-list');
  container.replaceChildren();
  for (const event of [...events].reverse().slice(0, 30)) {
    const row = node('div', 'event-row');
    row.append(node('span', 'event-time', timeLabel(event.time, true)),
      node('span', `event-kind ${event.kind}`, event.kind.toUpperCase()),
      node('span', 'event-detail', event.detail));
    if (event.pid) row.append(node('span', 'event-pid', `PID ${event.pid}`));
    container.append(row);
  }
}

function renderProcesses(data) {
  const container = $('#process-cards');
  container.replaceChildren();
  for (const process of data.processes) {
    const card = node('article', 'process-card');
    const header = node('div', 'process-title');
    header.append(node('span', `avatar avatar-${process.name.toLowerCase()}`, process.name), node('h3', '', `Process ${process.name}`), node('span', process.alive ? 'delivered' : 'failed', process.alive ? '● Online' : '● Offline'));
    const info = node('dl', 'process-data');
    for (const [label, value] of [['PROCESS ID', process.pid], ['STATE', process.state], ['RESIDENT MEMORY', `${process.rss_kb} KB`], ['FILE DESCRIPTORS', process.fd_count]]) {
      const pair = node('div'); pair.append(node('dt', '', label), node('dd', '', String(value))); info.append(pair);
    }
    card.append(header, info); container.append(card);
  }
  const fifoList = $('#fifo-list');
  fifoList.replaceChildren();
  for (const fifo of data.fifos) {
    const row = node('div', 'fifo-row');
    row.append(node('span', '', fifo.name === 'A_to_B' ? 'A → B' : 'B → A'), node('code', '', fifo.path), node('b', '', fifo.exists ? 'FIFO CREATED' : 'UNAVAILABLE'));
    fifoList.append(row);
  }
  const a = data.processes.find(p => p.name === 'A');
  const b = data.processes.find(p => p.name === 'B');
  commands = `# Inspect the independent C processes\nps -p ${a?.pid || 0},${b?.pid || 0} -o pid,ppid,stat,comm,args\n\n# Confirm the named pipe file types (look for p at the start)\nls -l '${data.runtime}'/*.fifo\n\n# Inspect open FIFO file descriptors\nls -l /proc/${a?.pid || 0}/fd /proc/${b?.pid || 0}/fd`;
  $('#terminal-commands').textContent = commands;
  $('#bridge-pid').textContent = data.server_pid;
  $('#uptime').textContent = `${Math.floor(data.uptime_seconds / 60)}m ${data.uptime_seconds % 60}s`;
}

function updateButtons() {
  const peersAlive = connected && state?.processes.length === 2 && state.processes.every(p => p.alive);
  $$('.composer').forEach(form => {
    const bytes = new TextEncoder().encode(form.querySelector('textarea').value).length;
    form.querySelector('.send-button').disabled = !peersAlive || sending.has(form.dataset.sender) || !form.querySelector('textarea').value.trim() || bytes > 1600;
  });
}

async function refresh() {
  try {
    state = await api('/api/state');
    connected = true;
    if (failureCount) notify('');
    failureCount = 0;
    $('#connection-dot').classList.add('online');
    $('#connection-label').textContent = 'Linux backend connected';
    $('#metric-delivered').textContent = state.stats.delivered;
    $('#sent-count').textContent = `${state.stats.sent} submitted this session`;
    const active = state.processes.filter(p => p.alive).length;
    $('#metric-processes').replaceChildren(document.createTextNode(String(active)), node('small', '', ' / 2'));
    $('#metric-latency').textContent = state.stats.average_latency_ms === null ? '—' : state.stats.average_latency_ms.toFixed(2);
    for (const peer of state.processes) {
      const lower = peer.name.toLowerCase();
      $(`#peer-${lower}-info`).textContent = `${peer.alive ? 'Online' : 'Offline'} · PID ${peer.pid}`;
      $(`[data-peer="${peer.name}"] .peer-dot`).classList.toggle('online', peer.alive);
    }
    if (active !== 2) notify('A Linux peer is offline. Open Process inspector and select Restart processes to restore communication.');
    renderMessages(state.messages);
    renderEvents(state.events);
    renderProcesses(state);
  } catch (error) {
    connected = false;
    failureCount += 1;
    $('#connection-dot').classList.remove('online');
    $('#connection-label').textContent = 'Backend disconnected';
    $$('.peer-dot').forEach(dot => dot.classList.remove('online'));
    if (failureCount >= 2) notify('Cannot reach the Linux backend. Keep the launcher terminal open, then reload this page when the server is running.');
  } finally {
    updateButtons();
  }
}

$$('.composer').forEach(form => {
  const input = form.querySelector('textarea');
  input.addEventListener('input', () => {
    const bytes = new TextEncoder().encode(input.value).length;
    form.querySelector('.byte-count').textContent = `${bytes} / 1600 B`;
    form.querySelector('.byte-count').classList.toggle('over', bytes > 1600);
    updateButtons();
  });
  input.addEventListener('keydown', event => {
    if (event.key === 'Enter' && !event.shiftKey && !event.isComposing) {
      event.preventDefault();
      if (!form.querySelector('button').disabled) form.requestSubmit();
    }
  });
  form.addEventListener('submit', async event => {
    event.preventDefault();
    const sender = form.dataset.sender;
    if (sending.has(sender) || !input.value.trim()) return;
    const text = input.value;
    sending.add(sender); updateButtons();
    try {
      await api('/api/send', {sender, text});
      // Preserve any new text the user typed while this message was submitting.
      if (input.value === text) { input.value = ''; input.dispatchEvent(new Event('input')); }
      notify('');
      await refresh();
    } catch (error) { notify(error.message); }
    finally { sending.delete(sender); updateButtons(); input.focus(); }
  });
});

$('#restart-button').addEventListener('click', async () => {
  const button = $('#restart-button');
  button.disabled = true;
  button.textContent = 'Restarting…';
  try { await api('/api/restart', {}); notify(''); await refresh(); }
  catch (error) { notify(error.message); }
  finally { button.disabled = false; button.textContent = '↻ Restart processes'; }
});

$('#copy-commands').addEventListener('click', async () => {
  try {
    await navigator.clipboard.writeText(commands);
    $('#copy-commands').textContent = 'Copied';
    setTimeout(() => { $('#copy-commands').textContent = 'Copy commands'; }, 1800);
  } catch (error) { notify('Select the commands in the terminal panel and copy them manually.'); }
});

async function poll() { await refresh(); setTimeout(poll, 500); }
updateButtons();
poll();
