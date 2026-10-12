// Native host pumping policy versus actual Node timer/Promise/microtask ordering.
// This verifies zero-delay scheduling only; it does not certify a DOM event loop.
// Uncaught timer exceptions use the documented native checkpoint/error policy;
// Node's uncaughtException recovery has different continuation ordering.
import { execFileSync } from 'node:child_process';
import assert from 'node:assert/strict';

const executable = process.argv[2];
if (!executable) throw new Error('Usage: node compare-native-event-loop.mjs <native-probe>');
const expected = [];
for (let kind = 0; kind < 4; ++kind) {
  const events = ['sync'], errors = [];
  await new Promise(done => {
    if (kind === 0) {
      setTimeout(() => { events.push('timer:A'); queueMicrotask(() => events.push('micro:A')); Promise.resolve().then(() => events.push('promise:A')); }, 0);
      setTimeout(() => { events.push('timer:B'); done(); }, 0);
      queueMicrotask(() => events.push('micro:script'));
      Promise.resolve().then(() => events.push('promise:script'));
    } else if (kind === 1) {
      setTimeout(() => {
        events.push('timer:A'); setTimeout(() => { events.push('timer:C'); done(); }, 0);
        queueMicrotask(() => events.push('micro:A'));
      }, 0);
      setTimeout(() => events.push('timer:B'), 0);
    } else if (kind === 2) {
      let canceled;
      setTimeout(() => { events.push('timer:A'); clearTimeout(canceled); queueMicrotask(() => events.push('micro:A')); }, 0);
      canceled = setTimeout(() => events.push('canceled'), 0);
      setTimeout(() => { events.push('timer:C'); done(); }, 0);
    } else if (kind === 3) {
      let resolve;
      new Promise(callback => { resolve = callback; }).then(() => {
        events.push('promise'); queueMicrotask(() => events.push('micro:promise'));
      });
      setTimeout(() => { events.push('timer:A'); resolve(); queueMicrotask(() => events.push('micro:A')); }, 0);
      setTimeout(() => { events.push('timer:B'); done(); }, 0);
    }
    events.push('return');
  });
  expected.push({ events, errors });
}
const actual = JSON.parse(execFileSync(executable, { encoding: 'utf8' }));
assert.deepStrictEqual(actual, expected);
console.log(`Native event loop: ${expected.length} Node order scenarios passed (${process.version}).`);
