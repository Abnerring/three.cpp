import { spawnSync, execFileSync } from 'node:child_process';
import fs from 'node:fs';
import { strict as assert } from 'node:assert';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [upstream, binary] = process.argv.slice(2);
if (!binary || !upstream) throw new Error('Usage: node compare-runtime-extended.mjs <three.js root> <probe>');
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const { REVISION } = await import(pathToFileURL(path.join(upstream, 'src/constants.js')).href);
assert.equal(REVISION, lock.revision);
const numberTrace = n => ({ type: 'number', value: Number.isNaN(n) ? 'NaN' :
    n === Infinity ? 'Infinity' : n === -Infinity ? '-Infinity' : Object.is(n, -0) ? '-0' : n });
const outcome = operation => { try { return operation(); } catch (e) { return { error: e.name }; } };
const scenarios = {};
{
    const payload = { answer: 42 }, fn = function fn(x) { return payload; }, alias = fn;
    scenarios.function = { identity: fn === alias, different: fn === (() => payload), captured: fn() === payload,
        // The C++ callback is a native function, so compare native own attributes.
        names: Object.getOwnPropertyNames(Array.prototype.join), keys: Object.keys(fn), name: fn.name,
        length: fn.length, callNonfunction: outcome(() => Reflect.apply(1, undefined, [])) };
}
{
    const prototype = { stored: 1 }, child = Object.create(prototype), other = { stored: 20 };
    child.stored = 10;
    Object.defineProperty(prototype, 'value', { get() { return this.stored; }, set(v) { this.stored = v; }, enumerable: true, configurable: true });
    const output = { inherited: 'value' in child, own: Object.hasOwn(child, 'value'), get: child.value,
        receiverGet: Reflect.get(prototype, 'value', other) };
    child.value = 11; Reflect.set(prototype, 'value', 21, other);
    Object.assign(output, { child: child.stored, other: other.stored, prototype: prototype.stored,
        keys: Object.keys(child), cycle: Reflect.setPrototypeOf(prototype, child) });
    Object.preventExtensions(child);
    Object.assign(output, { preventedPrototype: Reflect.setPrototypeOf(child, null), samePrototype: Reflect.setPrototypeOf(child, prototype),
        addPrevented: Reflect.set(child, 'new', 3), isExtensible: Object.isExtensible(child) });
    scenarios.prototype = output;
}
{
    const object = {};
    Object.defineProperty(object, 'zero', { value: -0 }); Object.defineProperty(object, 'nan', { value: NaN });
    const output = { positiveZero: Reflect.defineProperty(object, 'zero', { value: 0 }),
        negativeZero: Reflect.defineProperty(object, 'zero', { value: -0 }), NaN: Reflect.defineProperty(object, 'nan', { value: NaN }),
        erase: Reflect.deleteProperty(object, 'zero'), names: Object.getOwnPropertyNames(object), keys: Object.keys(object) };
    const child = Object.create(object);
    Object.assign(output, { inheritedReadonly: Reflect.set(child, 'zero', 2), ownAfterFailedSet: Object.hasOwn(child, 'zero'),
        strictSet: outcome(() => { child.zero = 2; return true; }),
        badGetter: outcome(() => Reflect.defineProperty(object, 'bad', { get: null })),
        mixedDescriptor: outcome(() => Reflect.defineProperty(object, 'bad', { get() { return 2; }, value: 2 })) });
    Object.defineProperty(object, 'missingGetter', { get: undefined, enumerable: true });
    Object.assign(output, { missingGetter: { type: 'undefined' }, missingSetter: Reflect.set(object, 'missingGetter', 2) });
    scenarios.descriptors = output;
}
{
    const array = []; array[0] = 0; array[2] = 2; array[4] = 4;
    Object.defineProperty(array, '2', { configurable: false });
    const output = { shrink: Reflect.defineProperty(array, 'length', { value: 1, writable: false }),
        length: array.length, keys: Object.keys(array), names: Object.getOwnPropertyNames(array),
        lengthWritable: Object.getOwnPropertyDescriptor(array, 'length').writable, extend: Reflect.set(array, '3', 3) };
    const numeric = Array(4), counter = { calls: 0 }, length = { valueOf() { ++counter.calls; return 2; } };
    numeric.length = length; output.coercedLength = numeric.length; output.calls = counter.calls;
    numeric.length = '3'; output.stringLength = numeric.length;
    output.fractionLength = outcome(() => { numeric.length = '3.5'; return true; });
    scenarios.arrayLength = output;
}
{
    const events = [], object = { valueOf() { events.push('valueOf'); return this; }, toString() { events.push('toString'); return '7'; } };
    const output = { number: Number(object), numberEvents: events.slice() };
    events.length = 0; output.string = String(object); output.stringEvents = events.slice();
    const prototype = { [Symbol.toPrimitive](hint) { return hint + ':' + this.label; } }, child = Object.create(prototype);
    child.label = 'child'; output.hookDefault = child + ''; output.hookString = String(child); output.hookNumber = numberTrace(Number(child));
    child[Symbol.toPrimitive] = () => child; output.hookObject = outcome(() => child + '');
    child[Symbol.toPrimitive] = 1; output.hookNoncallable = outcome(() => child + '');
    output.nullPrototype = outcome(() => numberTrace(Number(Object.create(null))));
    const array = Array(3); array[1] = null; array[2] = 4; output.sparseArray = String(array);
    const self = Array(2); self[0] = self; self[1] = 5; output.cyclicArray = String(self);
    output.emptyArrayNumber = Number([]); output.objectString = String({}); output.nativeString = String(Array.prototype.join);
    scenarios.coercion = output;
}
{
    const object = {};
    Object.defineProperty(object, 'first', { get() { this.added = 3; delete this.later; return 1; }, enumerable: true });
    object.later = 2;
    const output = { getterSnapshot: JSON.parse(JSON.stringify(object)) };
    const child = { toJSON(key) { return key; } };
    output.toJSONRoot = JSON.parse(JSON.stringify(child)); object.child = child; output.toJSONChild = JSON.parse(JSON.stringify(object));
    const array = Array(2); Object.defineProperty(array, '0', { get() { this[2] = 3; return 1; }, enumerable: true });
    output.arraySnapshot = JSON.parse(JSON.stringify(array)); output.arrayLengthAfter = array.length;
    output.functionArray = JSON.parse(JSON.stringify([() => 1])); scenarios.json = output;
}
{
    const scalar = '\u{1f4a9}', high = scalar[0], low = scalar[1], object = {};
    object[scalar] = 1; object[high + low] = 2;
    scenarios.utf16 = { length: scalar.length, identity: scalar === high + low,
        high: JSON.parse(JSON.stringify(high)), low: JSON.parse(JSON.stringify(low)),
        joined: JSON.parse(JSON.stringify(high + low)), keys: Object.keys(object), keyValue: object[scalar],
        erase: Reflect.deleteProperty(object, high + low), afterErase: Object.hasOwn(object, scalar) };
}

const lines = [], expected = [];
const addNumber = number => {
    const bytes = Buffer.alloc(8); bytes.writeDoubleLE(number);
    lines.push('N' + bytes.readBigUInt64LE().toString(16)); expected.push(String(number));
};
for (const n of [0, -0, NaN, Infinity, -Infinity, Number.MIN_VALUE, Number.MAX_VALUE,
    1e-7, 1e-6, 1e20, 1e21, 1e23, 1.2345678901234567, 1000000000000000100,
    9007199254740991, 9007199254740992, 2.2250738585072014e-308]) addNumber(n);
// Deterministic raw IEEE-754 values test shortest decimal formatting across exponents.
let state = 0x11738a9d55e502fbn;
const mask = (1n << 64n) - 1n;
for (let i = 0; i < 12000; ++i) {
    state ^= state << 13n; state &= mask; state ^= state >> 7n; state ^= state << 17n; state &= mask;
    const bytes = Buffer.alloc(8); bytes.writeBigUInt64LE(state); addNumber(bytes.readDoubleLE());
}
const strings = ['', ' ', '\t\n\r', '-0', '+0', '00', '0123', '.5', '1.', '+1.5e+2', '-1e-9999',
    '1e9999', 'Infinity', '+Infinity', '-Infinity', 'NaN', 'nan', 'inf', '-inf', '0x', '0x1', '0Xff',
    '0x20000000000001', '0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF', '0b11', '0o17', '0b102', '0o8',
    '+0x1', '-0x1', '0x1p2', '1_000', '12junk', '1e', '1e+', '.', '+', '--1', ' 1 2 ', '1n',
    '\u00a0\ufeff\u20281.25\u3000', '\u00851', '\u180e1', '\u200b1'];
for (const size of [9999, 10000, 10001, 32767]) {
    strings.push('9'.repeat(size) + 'e-' + size, '0.' + '0'.repeat(size) + '1e' + (size + 1),
        '-0.' + '0'.repeat(size) + '1e' + (size + 1), '0e' + '9'.repeat(size),
        '1e' + '9'.repeat(size), '1e-' + '9'.repeat(size), '0x' + '0'.repeat(size) + '1');
}
// Exact adjacent-double midpoints followed by a remote sticky digit expose
// parsers that incorrectly round after discarding a long decimal tail.
const dyadic = (integer, power) => {
    let digits = (integer * (5n ** BigInt(power))).toString();
    if (digits.length <= power) digits = '0'.repeat(power + 1 - digits.length) + digits;
    return digits.slice(0, digits.length - power) + '.' + digits.slice(digits.length - power);
};
for (const [numerator, power] of [[1n, 1075], [3n, 1075], [5n, 1075], [7n, 1075],
    [(1n << 53n) + 1n, 53], [(1n << 53n) + 3n, 53], [(1n << 53n) - 1n, 54]]) {
    const halfway = dyadic(numerator, power);
    strings.push(halfway, '-' + halfway);
    for (const tail of [40, 1300, 10000]) strings.push(halfway + '0'.repeat(tail) + '1', '-' + halfway + '0'.repeat(tail) + '1');
}
for (let i = 0; i < 800; ++i) {
    state ^= state << 13n; state &= mask; state ^= state >> 7n; state ^= state << 17n; state &= mask;
    const big = state * state * state + 1n;
    strings.push('0x' + big.toString(16), '0o' + big.toString(8), '0b' + big.toString(2),
        big.toString() + 'e' + ((i % 650) - 350));
}
for (const text of strings) { lines.push('S' + Buffer.from(text, 'utf8').toString('hex')); expected.push(numberTrace(Number(text))); }
const run = spawnSync(binary, [], { input: lines.join('\n') + '\n', encoding: 'utf8', maxBuffer: 32 * 1024 * 1024 });
if (run.error) throw run.error;
if (run.status !== 0) throw new Error(`Probe exited ${run.status}: ${run.stderr}`);
const actual = JSON.parse(run.stdout);
const failures = [];
const compare = (left, right, location) => {
    try { assert.deepEqual(left, right); } catch { failures.push({ location, cpp: left, js: right }); }
};
for (const key of Object.keys(scenarios)) compare(actual.scenarios[key], scenarios[key], key);
assert.equal(actual.conversions.length, expected.length);
for (let i = 0; i < expected.length; ++i) compare(actual.conversions[i], expected[i], `conversion[${i}]:${lines[i]}`);
if (failures.length) {
    console.error(JSON.stringify(failures.slice(0, 30), null, 2));
    throw new Error(`${failures.length} extended runtime mismatches`);
}
console.log(`Extended runtime parity: ${Object.keys(scenarios).length} scenarios and ${expected.length} conversions passed against Node ECMAScript (three.js ${REVISION}).`);
