import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { writeFileSync, mkdirSync } from 'node:fs';
import { pathToFileURL, fileURLToPath } from 'node:url';
import path from 'node:path';

const script = fileURLToPath(import.meta.url);
const child = process.argv[2] === '--child';
const upstream = process.argv[child ? 3 : 2];
if (!upstream) throw Error('compare-node-cache.mjs <pinned-three.js> <probe> [output-directory]');
const revision = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim();
assert.equal(revision, '11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const { default: NodeCache } = await import(pathToFileURL(path.join(upstream, 'src/nodes/core/NodeCache.js')));
const methods = ['getData', 'setData'];
const encode = value => value === undefined ? '#undefined' : typeof value === 'function' ? '#function' :
    typeof value === 'symbol' ? '#symbol' : value !== null && typeof value === 'object' ? '#object' : value;
const descriptor = d => ({ enumerable: d.enumerable, configurable: d.configurable,
    ...('value' in d ? { value: encode(d.value), writable: d.writable } :
        { get: typeof d.get === 'function', set: typeof d.set === 'function' }) });
const snapshot = value => value !== null && (typeof value === 'object' || typeof value === 'function') ?
    Object.fromEntries(Object.getOwnPropertyNames(value).map(key => [key, descriptor(Object.getOwnPropertyDescriptor(value, key))])) : {};
const attempt = fn => { try { fn(); return ''; } catch (error) { return error.name; } };
const accessor = (object, key, get, set) => Object.defineProperty(object, key,
    { get, set, configurable: true, enumerable: true });

if (child) {
    const mode = process.argv[4], retained = [];
    const refs = (() => {
        const parent = new NodeCache(), cache = new NodeCache(parent), map = cache.nodesData, key = {}, value = {};
        const refs = Object.fromEntries(Object.entries({ cache, map, key, value, parent }).map(([key, value]) => [key, new WeakRef(value)]));
        cache.setData(key, value);
        if (['retain-cache', 'retain-key', 'replace-map', 'delete'].includes(mode)) retained[0] = cache;
        if (['retain-key', 'key-only', 'replace-map', 'delete'].includes(mode)) retained[1] = key;
        if (mode === 'value-only') retained[0] = value;
        if (mode === 'cycle' || mode === 'retain-cache') { value.key = key; value.cache = cache; }
        if (mode === 'replace-map') cache.nodesData = new WeakMap();
        if (mode === 'delete') map.delete(key);
        return refs;
    })();
    for (let i = 0; i < 6; ++i) { await new Promise(resolve => setImmediate(resolve)); global.gc(); }
    console.log(JSON.stringify({ mode, ...Object.fromEntries(Object.entries(refs).map(([key, ref]) => [key, !!ref.deref()])), retained: retained.length }));
    process.exit(0);
}

function metadata() {
    const first = new NodeCache();
    return { kind: 'metadata', constructor: snapshot(NodeCache), prototype: Object.getOwnPropertyNames(NodeCache.prototype),
        constructorIdentity: NodeCache.prototype.constructor === NodeCache, prototypeIdentity: Object.getPrototypeOf(first) === NodeCache.prototype,
        call: attempt(() => NodeCache()), instance: snapshot(first), weakMap: first.nodesData instanceof WeakMap,
        methods: Object.fromEntries(methods.map(name => [name, { own: snapshot(NodeCache.prototype[name]),
            descriptor: descriptor(Object.getOwnPropertyDescriptor(NodeCache.prototype, name)),
            construct: attempt(() => Reflect.construct(NodeCache.prototype[name], [])) }])) };
}
function basic() {
    const parent = new NodeCache(), cache = new NodeCache(parent), key = {}, other = {}, rows = [];
    rows.push(encode(cache.getData(key))); rows.push(encode(parent.setData(key, 17))); rows.push(encode(cache.getData(key)));
    cache.setData(key, 8); rows.push(encode(cache.getData(key)));
    cache.setData(key, undefined); rows.push(encode(cache.getData(key)));
    cache.setData(key, null); rows.push(encode(cache.getData(key)));
    cache.setData(key, false); rows.push(encode(cache.getData(key)));
    rows.push(encode(cache.getData(other)));
    cache.nodesData = new WeakMap(); rows.push(encode(cache.getData(key)));
    cache.parent = null; rows.push(encode(cache.getData(key)));
    const data = { tag: 3 }; cache.setData(key, data); const identity = cache.getData(key) === data;
    cache.getData = function replacement(argument) { return this === cache && argument === key; };
    rows.push(encode(cache.getData(key)));
    cache.setData = function replacement(argument, value) { return this === cache && argument === key && value === data; };
    rows.push(encode(cache.setData(key, data)));
    return { kind: 'basic', idGap: cache.id - parent.id, sameParent: cache.parent === null, dataIdentity: identity, steps: rows };
}
function keys(index) {
    const key = [{}, [], () => {}, undefined, null, false, 7, 'key', Symbol('key')][index];
    const cache = new NodeCache(); let result;
    const setError = attempt(() => result = cache.setData(key, 23));
    const getError = attempt(() => result = cache.getData(key));
    return { kind: 'keys', index, setError, getError, value: encode(result) };
}
function dynamic(op, mode) {
    const self = {}, map = {}, parent = {}, key = {}, data = {}, events = []; let reads = 0;
    accessor(self, 'nodesData', function () {
        events.push(['nodesData.get', this === self]);
        if (mode === 8) throw RangeError('nodesData getter');
        if (mode === 18) return null;
        if (mode === 19) Object.defineProperty(self, 'nodesData', { value: null, writable: true, enumerable: true, configurable: true });
        return map;
    });
    accessor(self, 'parent', function () {
        const count = reads++; events.push(['parent.get', this === self, count]);
        if (mode === 11 || (mode === 12 && count === 1)) throw RangeError('parent getter');
        if ([1, 5, 7, 21].includes(mode)) return null;
        if (mode === 6 && count > 0) return null;
        if (mode === 15) return undefined;
        return parent;
    });
    for (const name of ['get', 'set']) accessor(map, name, function () {
        events.push([name + '.get', this === map]);
        if (mode === 9) throw RangeError('map method getter');
        if (mode === 16) return 7;
        return function (...args) {
            events.push([name + '.call', this === map, args.length, args[0] === key, args.length > 1 && args[1] === data]);
            if (mode === 10) throw RangeError('map method');
            if (name === 'set') return 'ignored-map-result';
            if (mode === 0) return 9;
            if (mode === 3) return null;
            if (mode === 4) return false;
            if (mode === 20) parent.getData = function () { events.push('replacement.call'); return 99; };
            return undefined;
        };
    });
    if (mode !== 20) accessor(parent, 'getData', function () {
        events.push(['getData.get', this === parent]);
        if (mode === 13) throw RangeError('parent method getter');
        if (mode === 17) return 7;
        return function (...args) {
            events.push(['getData.call', this === parent, args.length, args[0] === key]);
            if (mode === 14) throw RangeError('parent method');
            return 31;
        };
    });
    let result; const error = attempt(() => result = NodeCache.prototype[op].call(self, key, data));
    return { kind: 'dynamic', op, mode, error, result: encode(result), events };
}
function construct(mode) {
    const before = new NodeCache().id, events = [], prototype = {};
    function Other() {} Other.prototype = prototype;
    if (mode === 8 || mode === 12) for (const name of ['id', 'nodesData', 'parent']) accessor(prototype, name, undefined, function (value) {
        events.push([name + '.set', Object.getPrototypeOf(this) === prototype, name === 'id' ? 'id' : value !== null && typeof value === 'object' ? 'object' : value]);
        if (mode === 12 && name === 'id') events.push(['nested', new NodeCache().id - value]);
    });
    if (mode >= 9 && mode <= 11) Object.defineProperty(prototype, mode === 9 ? 'id' : mode === 10 ? 'nodesData' : 'parent',
        { value: 77, enumerable: true, configurable: true });
    const parent = mode === 3 ? false : mode === 4 ? 0 : mode === 5 ? 'text' : mode === 6 ? {} : null;
    const args = mode === 0 ? [] : mode === 1 ? [undefined] : [parent]; let result;
    const error = attempt(() => result = mode >= 7 ? Reflect.construct(NodeCache, args, Other) : Reflect.construct(NodeCache, args));
    const after = new NodeCache().id, own = snapshot(result);
    if (result && Object.hasOwn(result, 'id')) own.id.value = result.id - before;
    return { kind: 'construct', mode, error, own, samePrototype: result !== undefined && Object.getPrototypeOf(result) === prototype,
        sameParent: result !== undefined && result.parent === parent, idAdvance: after - before, events };
}

const expected = [metadata(), basic(), ...Array.from({ length: 9 }, (_, index) => keys(index))];
for (const op of methods) for (let mode = 0; mode < 22; ++mode) expected.push(dynamic(op, mode));
for (let mode = 0; mode < 13; ++mode) expected.push(construct(mode));
const probe = process.argv[3], output = process.argv[4];
if (!probe) { console.log(JSON.stringify(expected)); process.exit(0); }
const actual = execFileSync(probe, { encoding: 'utf8', maxBuffer: 16 * 1024 * 1024, timeout: 60000 }).trim().split(/\r?\n/).map(JSON.parse);
const expectedGc = [], actualGc = [];
for (const mode of ['empty', 'retain-cache', 'retain-key', 'key-only', 'value-only', 'cycle', 'replace-map', 'delete']) {
    expectedGc.push(JSON.parse(execFileSync(process.execPath, ['--expose-gc', script, '--child', upstream, mode], { encoding: 'utf8', timeout: 30000 })));
    actualGc.push(JSON.parse(execFileSync(probe, [mode], { encoding: 'utf8', timeout: 30000 })));
}
if (output) {
    mkdirSync(output, { recursive: true });
    for (const [name, value] of Object.entries({ expected, actual, expectedGc, actualGc })) writeFileSync(path.join(output, name + '.json'), JSON.stringify(value, null, 2));
}
assert.deepEqual(actual, expected, 'NodeCache protocol');
assert.deepEqual(actualGc, expectedGc, 'NodeCache ephemeron lifetime snapshots');
console.log(`NodeCache parity: ${expected.length} protocol scenarios and ${expectedGc.length} actual Node GC snapshots passed (${process.version}, ${revision}).`);
