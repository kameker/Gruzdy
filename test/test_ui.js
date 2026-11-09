const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

const html = fs.readFileSync('src/index.html', 'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements = Object.fromEntries(['revs', 'status', 'start', 'direction', 'minus', 'plus', 'stop'].map(id => [id, { value: '5', disabled: false, textContent: '' }]));
let resolveStop;
const stopResponse = new Promise(resolve => { resolveStop = resolve; });
let token = '123-0';
let stopCalls = 0;
const response = data => ({ ok: true, json: async () => data, text: async () => '' });
const context = {
  document: { getElementById: id => elements[id] },
  URLSearchParams, AbortController,
  setTimeout: () => 0, clearTimeout: () => {},
  fetch: async (url) => {
    if (url === '/status') return response({ busy: false, ready: true, token });
    if (url === '/move') throw new Error('response lost');
    if (url === '/stop') { stopCalls++; return stopResponse; }
    throw new Error('unexpected request');
  },
};
const flush = () => new Promise(resolve => setImmediate(resolve));

(async () => {
  vm.runInNewContext(script, context);
  await flush();
  const start = elements.start.onclick();
  await flush();
  assert.equal(elements.start.disabled, true, 'start must remain disabled after an uncertain move');
  assert.equal(stopCalls, 1, 'an uncertain move must be cancelled before re-enabling start');
  token = '123-1';
  resolveStop(response());
  await start;
  await flush();
  assert.equal(elements.start.disabled, false);
  console.log('UI recovery: PASS');

  let finishMove;
  const lateMove = new Promise(resolve => { finishMove = resolve; });
  let revision = '123-0';
  const other = Object.fromEntries(Object.keys(elements).map(id => [id, { value: '5', disabled: false, textContent: '' }]));
  const second = {
    ...context,
    document: { getElementById: id => other[id] },
    fetch: async url => {
      if (url === '/status') return response({ busy: false, ready: true, token: revision });
      if (url === '/move') return lateMove;
      if (url === '/stop') { revision = '123-1'; return response(); }
      throw new Error('unexpected request');
    },
  };
  vm.runInNewContext(script, second);
  await flush();
  const lateStart = other.start.onclick();
  await flush();
  await other.stop.onclick();
  assert.equal(other.start.disabled, true, 'do not re-enable while the old start is in flight');
  finishMove(response());
  await lateStart;
  assert.equal(other.start.disabled, false);
  assert.equal(other.status.textContent, 'Двигатель остановлен.');
  console.log('Stop while starting: PASS');

  const thirdElements = Object.fromEntries(Object.keys(elements).map(id => [id, { value: '5', disabled: false, textContent: '' }]));
  const third = {
    ...context,
    document: { getElementById: id => thirdElements[id] },
    fetch: async url => {
      if (url === '/status') return response({ busy: false, ready: true, token: '123-0' });
      throw new Error('connection lost');
    },
  };
  vm.runInNewContext(script, third);
  await flush();
  await thirdElements.start.onclick();
  assert.equal(thirdElements.start.disabled, true, 'failed stop must keep start locked');
  assert.match(thirdElements.status.textContent, /не удалось подтвердить остановку/i);
  console.log('Unconfirmed stop: PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
