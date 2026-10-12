import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
const executable = process.argv[2];
if (!executable) throw Error('Usage: node compare-native-weakmap.mjs <native-probe>');
const values = () => [undefined, null, false, true, 0, -0, 1.5, NaN, Infinity, -Infinity, '', 'value', {}, [], function(){}];
const key = kind => kind === 0 ? {} : kind === 1 ? [] : function(){};
const encode = value => value === undefined ? 'undefined' : value === null ? null
  : typeof value === 'number' ? Number.isNaN(value) ? 'NaN' : !Number.isFinite(value) ? value < 0 ? '-Infinity' : 'Infinity' : Object.is(value, -0) ? '-0' : value
  : typeof value === 'function' ? 'function' : typeof value === 'object' ? Array.isArray(value) ? 'array' : 'object' : value;
const invoke = callback => { try { return encode(callback()); } catch(error) { return {error: error.name}; } };
const expected = [];
for (let kind = 0; kind < 3; kind++) for (let index = 0; index < 15; index++) {
  const map = new WeakMap(), object = key(kind), value = values()[index];
  const initial = map.has(object), missing = map.get(object), set = map.set(object, value), present = map.has(object), get = map.get(object);
  const same = Object.is(get, value), unrelated = map.get(key(kind)), removed = map.delete(object), again = map.delete(object), after = map.has(object);
  expected.push({kind:'identity', key:kind, value:index, initial:encode(initial), missing:encode(missing), returnedSelf:set === map,
    present:encode(present), get:encode(get), same, unrelated:encode(unrelated), removed:encode(removed), again:encode(again), after:encode(after)});
}
for (let index = 0; index < 12; index++) {
  const map = new WeakMap(), object = values()[index]; expected.push({kind:'primitive', index,
    has:invoke(() => map.has(object)), get:invoke(() => map.get(object)), delete:invoke(() => map.delete(object)), set:invoke(() => map.set(object, 1))});
}
for (const method of ['get', 'has', 'delete', 'set']) for (let index = 0; index < 15; index++) {
  const receiver = values()[index]; expected.push({kind:'brand', method, index, result:invoke(() => WeakMap.prototype[method].call(receiver, null, 1))});
}
for (const method of ['get', 'has', 'delete', 'set']) {
  const fn = WeakMap.prototype[method], d = Object.getOwnPropertyDescriptor(WeakMap.prototype, method);
  expected.push({kind:'descriptor', method, name:encode(fn.name), length:encode(fn.length), writable:d.writable, enumerable:d.enumerable, configurable:d.configurable});
}
expected.push({kind:'constructor', name:encode(WeakMap.name), length:encode(WeakMap.length), call:invoke(() => WeakMap()),
  null:new WeakMap(null) instanceof WeakMap, undefined:new WeakMap(undefined) instanceof WeakMap, prototypeIsMap:false});
function NewTarget(){};
NewTarget.prototype = 17; const fallback = Reflect.construct(WeakMap, [], NewTarget), custom = {};
NewTarget.prototype = custom; const derived = Reflect.construct(WeakMap, [], NewTarget);
expected.push({kind:'newTarget', fallback:Object.getPrototypeOf(fallback) === WeakMap.prototype, custom:Object.getPrototypeOf(derived) === custom,
  brand:WeakMap.prototype.has.call(derived, {}) === false, missing:encode(WeakMap.prototype.get.call(derived, {}))});
const map = new WeakMap(), events = [], object = {[Symbol.toPrimitive]() { events.push('coerced'); throw RangeError('key'); }};
map.set(object, object); expected.push({kind:'noCoercion', same:map.get(object) === object, has:encode(map.has(object)), delete:encode(map.delete(object)), conversions:events.length});
const actual = execFileSync(executable, {encoding:'utf8'}).trim().split(/\r?\n/).map(line => JSON.parse(line));
assert.deepEqual(actual, expected);
let comparisons = 0; const count = value => { comparisons++; if (value && typeof value === 'object') for (const child of Object.values(value)) count(child); };
expected.forEach(count);
console.log(`Native WeakMap: ${expected.length} Node scenarios / ${comparisons} comparisons passed (${process.version}).`);
