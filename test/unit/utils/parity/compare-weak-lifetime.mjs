import { execFileSync } from 'node:child_process';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';
if (!global.gc) {
  const result = execFileSync(process.execPath, ['--expose-gc', fileURLToPath(import.meta.url), ...process.argv.slice(2)], { encoding: 'utf8' });
  process.stdout.write(result);
  process.exit(0);
}
const executable = process.argv[2];
if (!executable) throw new Error('Pass JSWeakLifetimeParityProbe executable');
const actual = execFileSync(executable, [], { encoding: 'utf8' }).trim().split(/\r?\n/).map(JSON.parse);
const expected = [];
const emit = row => expected.push(row);
const noop = () => {};
const values = () => [undefined, null, false, true, 0, -0, 1.5, NaN, Infinity, -Infinity, '', 'value', {}, [], noop];
const encode = value => {
  if (value === undefined) return 'undefined';
  if (value === null || typeof value === 'boolean' || typeof value === 'string') return value;
  if (typeof value === 'number') return Number.isNaN(value) ? 'NaN' : Object.is(value, -0) ? '-0' : Number.isFinite(value) ? value : value < 0 ? '-Infinity' : 'Infinity';
  return typeof value === 'function' ? 'function' : Array.isArray(value) ? 'array' : 'object';
};
const invoke = callback => {
  try { return encode(callback()); } catch (error) { if (!(error instanceof TypeError)) throw error; return { error: 'TypeError' }; }
};
const constructors = [WeakRef, FinalizationRegistry];
for (let c = 0; c < 2; ++c) {
  const ctor = constructors[c], prototype = ctor.prototype, d = Object.getOwnPropertyDescriptor(ctor, 'prototype');
  emit({ kind: 'constructor', index: c, name: ctor.name, length: ctor.length, call: invoke(() => ctor()), constructor: prototype.constructor === ctor,
    writable: d.writable, enumerable: d.enumerable, configurable: d.configurable });
  for (let i = 0; i < 15; ++i) emit({ kind: 'construct', constructor: c, index: i, result: invoke(() => new ctor(values()[i])) });
  function Target() {}
  Target.prototype = 3;
  const argument = c === 0 ? {} : noop, fallback = Reflect.construct(ctor, [argument], Target), custom = {};
  Target.prototype = custom; const derived = Reflect.construct(ctor, [argument], Target);
  const branded = c === 0 ? WeakRef.prototype.deref.call(derived) === argument : FinalizationRegistry.prototype.unregister.call(derived, {}) === false;
  emit({ kind: 'newTarget', constructor: c, fallback: Object.getPrototypeOf(fallback) === prototype, custom: Object.getPrototypeOf(derived) === custom, brand: branded });
}
for (const [ctor, name] of [[WeakRef, 'deref'], [FinalizationRegistry, 'register'], [FinalizationRegistry, 'unregister']]) {
  const method = ctor.prototype[name], d = Object.getOwnPropertyDescriptor(ctor.prototype, name);
  emit({ kind: 'method', method: name, name: method.name, length: method.length, construct: invoke(() => new method()), prototype: Object.hasOwn(method, 'prototype'),
    writable: d.writable, enumerable: d.enumerable, configurable: d.configurable });
  for (let i = 0; i < 15; ++i) emit({ kind: 'brand', method: name, index: i, result: invoke(() => method.call(values()[i], null)) });
}
for (let kind = 12; kind < 15; ++kind) {
  const target = values()[kind], ref = new WeakRef(target);
  emit({ kind: 'identity', index: kind, same: ref.deref() === target, ownKeys: Object.keys(ref).length,
    prototypeBrand: (() => { try { WeakRef.prototype.deref.call(WeakRef.prototype); return true; } catch { return false; } })() });
}
const registry = new FinalizationRegistry(noop), all = values();
for (let target = 0; target < 15; ++target) for (let held = 0; held < 15; ++held) for (let token = 0; token < 15; ++token)
  emit({ kind: 'register', target, held, token, result: invoke(() => registry.register(all[target], all[held], all[token])) });
for (let token = 0; token < 15; ++token) emit({ kind: 'unregister', token,
  first: invoke(() => registry.unregister(all[token])), again: invoke(() => registry.unregister(all[token])) });
{
  const log = [], target = { [Symbol.toPrimitive]() { log.push(1); throw new TypeError('coercion'); } }, ref = new WeakRef(target);
  registry.register(target, 'held', target);
  emit({ kind: 'noCoercion', same: ref.deref() === target, removed: registry.unregister(target), conversions: log.length });
}
const nextJob = () => new Promise(resolve => setImmediate(resolve));
// Never compare the host's chosen cleanup order or number of elapsed turns.
// Each deref observation is followed by a boundary before the next GC attempt.
async function collectUntil(predicate, label) {
  for (let i = 0; i < 100; ++i) {
    await nextJob(); global.gc(); await nextJob();
    if (predicate()) return;
  }
  throw new Error('Node GC/cleanup did not reach the required state: ' + label);
}
{
  let target = {}; const ref = new WeakRef(target); target = null; global.gc();
  const sameJob = ref.deref() !== undefined;
  const microtask = await Promise.resolve().then(() => { global.gc(); return ref.deref() !== undefined; });
  await collectUntil(() => ref.deref() === undefined, 'WeakRef target');
  emit({ kind: 'gcKeepalive', sameJob, microtask, nextJobDead: ref.deref() === undefined });
}
{
  const log = [], held = { marker: 42 };
  const reg = new FinalizationRegistry(function (value) { log.push(this === undefined, value === held, value.marker); });
  let target = {}; const ref = new WeakRef(target); reg.register(target, held); target = null;
  const syncEmpty = log.length === 0;
  await nextJob(); global.gc(); const queuedEmpty = log.length === 0;
  await collectUntil(() => log.length === 3 && ref.deref() === undefined, 'cleanup identity');
  emit({ kind: 'gcCleanup', syncEmpty, queuedEmpty, dead: ref.deref() === undefined, receiver: log[0], identity: log[1], marker: log[2], count: log.length / 3 });
}
{
  const log = [], token = {}, reg = new FinalizationRegistry(value => log.push(true, value));
  let target = {}; const ref = new WeakRef(target); reg.register(target, 'a', token); reg.register(target, 'b', token);
  const removed = reg.unregister(token); target = null;
  await collectUntil(() => ref.deref() === undefined, 'unregistered target');
  emit({ kind: 'gcUnregister', removed, dead: ref.deref() === undefined, calls: log.length });
}
{
  const log = [], token = {}, reg = new FinalizationRegistry(value => log.push(true, value));
  let target = {}; const ref = new WeakRef(target); reg.register(target, 'held', token); target = null;
  await nextJob(); global.gc();
  const removed = reg.unregister(token);
  await collectUntil(() => ref.deref() === undefined, 'pending unregister target');
  emit({ kind: 'gcPendingUnregister', removed, dead: ref.deref() === undefined, calls: log.length });
}
{
  const log = [], token = {}, reg = new FinalizationRegistry(value => log.push(true, value));
  let target = {}, held = { target }; const ref = new WeakRef(target);
  reg.register(target, held, token); target = null; held = null; await nextJob(); global.gc();
  const alive = ref.deref() !== undefined;
  reg.unregister(token); await collectUntil(() => ref.deref() === undefined, 'held backreference removed');
  emit({ kind: 'gcHeldReachability', alive, afterUnregisterDead: ref.deref() === undefined, calls: log.length });
}
{
  const log = [];
  let reg = new FinalizationRegistry(value => log.push(true, value)), target = {}, held = { registry: reg };
  const refRegistry = new WeakRef(reg), refHeld = new WeakRef(held);
  reg.register(target, held); reg = null; held = null; target = null;
  await collectUntil(() => refRegistry.deref() === undefined && refHeld.deref() === undefined, 'registry/held cycle');
  emit({ kind: 'gcRegistryCycle', registryDead: refRegistry.deref() === undefined, heldDead: refHeld.deref() === undefined, calls: log.length });
}
{
  const log = [], reg = new FinalizationRegistry(value => log.push(true, value));
  let target = {}, token = {}; const refToken = new WeakRef(token);
  reg.register(target, 'held', token); token = null;
  await collectUntil(() => refToken.deref() === undefined, 'weak unregister token');
  const tokenDead = refToken.deref() === undefined; target = null;
  await collectUntil(() => log.length === 2, 'cleanup after token collection');
  emit({ kind: 'gcWeakToken', tokenDead, calls: log.length / 2, held: log[1] });
}
assert.equal(actual.length, expected.length);
let comparisons = 0;
for (let i = 0; i < expected.length; ++i) {
  assert.deepEqual(actual[i], expected[i], 'Scenario ' + i + ' (' + expected[i].kind + ')');
  comparisons += Object.keys(expected[i]).length;
}
console.log('Weak lifetime parity passed: ' + expected.length + ' scenarios / ' + comparisons + ' fields against Node ' + process.version + ' --expose-gc.');
