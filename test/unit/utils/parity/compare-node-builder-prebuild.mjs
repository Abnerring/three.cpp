import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
const upstream = path.resolve(process.argv[2]);
const binary = process.argv[3] && process.argv[3] !== '--oracle' ? path.resolve(process.argv[3]) : undefined;
const outputDirectory = process.argv[4] && path.resolve(process.argv[4]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const { default: NodeBuilder } = await import(pathToFileURL(path.join(upstream, 'src/nodes/core/NodeBuilder.js')));
const { getConsoleFunction, setConsoleFunction } = await import(pathToFileURL(path.join(upstream, 'src/utils.js')));
const enc = v => v === undefined ? '#undefined' : typeof v === 'symbol' ? '#symbol' : typeof v === 'function' ? '#function' : v !== null && typeof v === 'object' ? '#object' : v;
const decode = v => v?.tag === 'undefined' ? undefined : v;
function view(value) {
    if (value === null || typeof value !== 'object') return enc(value);
    return Reflect.ownKeys(value).map(key => { const d = Object.getOwnPropertyDescriptor(value, key); return [typeof key === 'symbol' ? '#symbol' : key, 'value' in d ? enc(d.value) : '#accessor', !!d.writable, !!d.enumerable, !!d.configurable]; });
}
function run(r) {
    const events = [], counts = {}, symbol = Symbol('context'), nodes = {}, self = {}, renderer = {}, material = {}, library = {}, nodeMaterial = {}, compute = {}, flows = [];
    const fail = (label, count) => { if (r.throwAt === label || r.throwAt === label + ':' + count) throw new TypeError(label); };
    const get = (object, key, label, value) => Object.defineProperty(object, key, { configurable: true, enumerable: true, get() {
        const n = counts[label] = (counts[label] || 0) + 1; events.push(['get', label, n, this === object]); fail(label, n); return typeof value === 'function' ? value(n) : value;
    } });
    const method = (object, key, label, action) => { const fn = function (...args) { const n = counts[label + ':call'] = (counts[label + ':call'] || 0) + 1; events.push(['call', label, this === object, args.map(enc)]); fail(label + ':call', n); return action(args); }; get(object, key, label, () => fn); };
    function context(label) {
        const object = {}; Object.defineProperty(object, '2', { value: label + '-index', enumerable: true, configurable: true, writable: true });
        Object.defineProperty(object, 'first', { configurable: true, enumerable: true, get() { events.push(['spread', label, 'first']); fail(label + '.first', 1);
            if (r.mutateSource === label) {
                if (r.mutation === 'delete') delete object.later;
                if (r.mutation === 'replace') Object.defineProperty(object, 'later', { value: 'replaced', enumerable: true, configurable: true, writable: true });
                if (r.mutation === 'hidden') Object.defineProperty(object, 'hidden', { value: 'visible-now', enumerable: true, configurable: true, writable: true });
                if (r.mutation === 'symbol') delete object[symbol];
                object.added = 'added-late';
            }
            return label + '-first';
        } });
        Object.defineProperty(object, 'overlap', { value: label, enumerable: true, configurable: true, writable: true });
        Object.defineProperty(object, 'later', { configurable: true, enumerable: true, get() { events.push(['spread', label, 'later']); fail(label + '.later', 1); return label + '-later'; } });
        Object.defineProperty(object, 'hidden', { value: 'hidden', enumerable: false, configurable: true, writable: true });
        Object.defineProperty(object, '__proto__', { value: label + '-proto', enumerable: true, configurable: true, writable: true });
        Object.defineProperty(object, symbol, { configurable: true, enumerable: r.symbolEnumerable !== false, get() { events.push(['spread', label, '#symbol']); fail(label + '.symbol', 1); return label + '-symbol'; } }); return object;
    }
    const initial = context('initial'), rendererData = context('renderer'), materialData = context('material'); let current = initial;
    const data = label => {
        const mode = r[label + 'Data']; if (mode === 'null') return null; if (mode === 'undefined') return undefined; if (mode === 'string') return 'A\ud83d\ude00'; if (mode === 'number') return 4;
        return label === 'renderer' ? rendererData : materialData;
    };
    for (const label of ['renderer', 'material', 'other']) {
        const object = nodes[label] = {};
        get(object, 'isContextNode', label + '.isContextNode', Object.hasOwn(r, label + 'Flag') ? decode(r[label + 'Flag']) : true);
        method(object, 'getFlowContextData', label + '.getFlowContextData', () => data(label === 'other' ? 'renderer' : label));
    }
    const select = value => value === 'null' ? null : value === 'undefined' ? undefined : value === 'false' ? false : value === 'zero' ? 0 : nodes[value];
    get(renderer, 'contextNode', 'renderer.contextNode', n => select((r.rendererSequence || ['renderer'])[Math.min(n - 1, (r.rendererSequence || ['renderer']).length - 1)]));
    get(material, 'contextNode', 'material.contextNode', n => select((r.materialSequence || ['material'])[Math.min(n - 1, (r.materialSequence || ['material']).length - 1)]));
    get(renderer, 'library', 'renderer.library', library);
    method(library, 'fromMaterial', 'library.fromMaterial', () => r.fromResult === 'false' ? false : r.fromResult === 'undefined' ? undefined : nodeMaterial);
    method(nodeMaterial, 'build', 'nodeMaterial.build', args => { events.push(['buildState', args[0] === self, view(current)]); current.built = true; });
    get(self, 'renderer', 'self.renderer', renderer);
    const materialValue = r.material === 'null' ? null : r.material === 'undefined' ? undefined : r.material === 'false' ? false : r.material === 'zero' ? 0 : material;
    get(self, 'material', 'self.material', materialValue);
    Object.defineProperty(self, 'context', { configurable: true, enumerable: true, get() { const n = counts['self.context'] = (counts['self.context'] || 0) + 1; events.push(['get', 'self.context', n, this === self]); fail('self.context', n); return current; },
        set(value) { const n = counts['self.context:set'] = (counts['self.context:set'] || 0) + 1; events.push(['set', 'self.context', n, this === self, view(value)]); fail('self.context:set', n); current = value; } });
    get(self, 'compute', 'self.compute', compute);
    method(self, 'addFlow', 'self.addFlow', args => { flows.push([args[0], args[1] === compute]); });
    const saved = getConsoleFunction(); setConsoleFunction(function (...args) { events.push(['console', args.map(enc)]); fail('console', 1); });
    let result;
    try { try { result = enc(NodeBuilder.prototype.prebuild.call(self)); } catch (e) { result = { error: e.name }; } }
    finally { setConsoleFunction(saved); }
    return { result, events, context: view(current), initialIdentity: current === initial, flows, prototypeOrdinary: Object.getPrototypeOf(current) === Object.prototype };
}
const cases = [];
for (const material of ['object', 'null', 'undefined', 'false', 'zero']) for (const rendererFlag of [true, false, 1, 'true', null, { tag: 'undefined' }]) cases.push({ material, rendererFlag });
for (const materialFlag of [true, false, 1, 'true', null, { tag: 'undefined' }]) for (const symbolEnumerable of [false, true]) cases.push({ materialFlag, symbolEnumerable });
for (const rendererSequence of [['null'], ['undefined'], ['false'], ['zero'], ['renderer', 'other'], ['renderer', 'null'], ['renderer', 'undefined']]) cases.push({ rendererSequence });
for (const materialSequence of [['null'], ['undefined'], ['false'], ['zero'], ['material', 'other', 'material'], ['material', 'material', 'other'], ['material', 'null'], ['material', 'material', 'undefined']]) cases.push({ materialSequence });
for (const label of ['renderer', 'material']) for (const mode of ['null', 'undefined', 'string', 'number']) cases.push({ [label + 'Data']: mode });
for (const mutateSource of ['initial', 'renderer', 'material']) for (const mutation of ['delete', 'replace', 'hidden', 'symbol']) cases.push({ mutateSource, mutation });
for (const fromResult of ['false', 'undefined']) cases.push({ fromResult });
for (const throwAt of ['self.renderer', 'self.material', 'renderer.contextNode:1', 'renderer.contextNode:2', 'renderer.isContextNode', 'renderer.getFlowContextData', 'renderer.getFlowContextData:call',
    'self.context:1', 'self.context:2', 'self.context:set:1', 'self.context:set:2', 'initial.first', 'initial.later', 'initial.symbol', 'renderer.first', 'renderer.later', 'renderer.symbol',
    'material.contextNode:1', 'material.contextNode:2', 'material.contextNode:3', 'material.isContextNode', 'material.getFlowContextData', 'material.getFlowContextData:call', 'material.first', 'material.later', 'material.symbol',
    'renderer.library', 'library.fromMaterial', 'library.fromMaterial:call', 'nodeMaterial.build', 'nodeMaterial.build:call']) cases.push({ throwAt });
for (const throwAt of ['self.addFlow', 'self.addFlow:call', 'self.compute']) cases.push({ material: 'null', throwAt });
cases.push({ rendererFlag: false, throwAt: 'console' }, { materialFlag: false, throwAt: 'console' });
const expected = cases.map(run);
if (outputDirectory) { fs.mkdirSync(outputDirectory, { recursive: true }); for (const [name, value] of [['input', cases], ['expected', expected]]) fs.writeFileSync(path.join(outputDirectory, 'node-builder-prebuild-' + name + '.json'), JSON.stringify(value, null, 2)); }
if (binary) {
    const actual = JSON.parse(execFileSync(binary, [], { input: JSON.stringify(cases), encoding: 'utf8', maxBuffer: 256 * 1024 * 1024 })); assert.equal(actual.length, expected.length);
    const differences = []; for (let i = 0; i < cases.length; ++i) try { assert.deepEqual(actual[i], expected[i]); } catch { differences.push({ index: i, request: cases[i], expected: expected[i], actual: actual[i] }); }
    if (outputDirectory) for (const [name, value] of [['actual', actual], ['differences', differences]]) fs.writeFileSync(path.join(outputDirectory, 'node-builder-prebuild-' + name + '.json'), JSON.stringify(value, null, 2));
    assert.equal(differences.length, 0, `${differences.length} prebuild differences; first ${JSON.stringify(differences[0])}`);
}
console.log(`NodeBuilder.prebuild ${binary ? 'parity' : 'real upstream oracle'}: ${cases.length} scenarios; NodeMaterial fallback remains pending`);
