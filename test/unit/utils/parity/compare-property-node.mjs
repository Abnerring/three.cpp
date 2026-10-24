import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream = path.resolve(process.argv[2]), binary = process.argv[3] && process.argv[3] !== '--oracle' ? path.resolve(process.argv[3]) : undefined, output = process.argv[4] && path.resolve(process.argv[4]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const load = async name => (await import(pathToFileURL(path.join(upstream, 'src/nodes/core/' + name + '.js')))).default;
const [PropertyNode, ParameterNode] = await Promise.all(['PropertyNode', 'ParameterNode'].map(load));
const {nodeObject, getConstNodeType} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLCore.js')));
const enc = v => v === undefined ? '#undefined' : typeof v === 'symbol' ? '#symbol' : typeof v === 'function' ? '#function' : v !== null && typeof v === 'object' ? '#object' : Number.isNaN(v) ? '#NaN' : v === Infinity ? '#Infinity' : v === -Infinity ? '#-Infinity' : Object.is(v, -0) ? '#-0' : v;
function normal(v) { if (v === null || typeof v !== 'object') return enc(v); if (v.isNode === true) return v.isConstNode ? {const: true, type: v.nodeType, value: enc(v.value)} : {node: true, type: v.type}; if (v._fixtureTag) return '#' + v._fixtureTag; if (Array.isArray(v)) return v.map(normal); return Object.fromEntries(Object.keys(v).map(k => [k, normal(v[k])])); }
function decode(v, s) { if (v === null || typeof v !== 'object') return v; if (v.tag === 'undefined') return undefined; if (v.tag === 'symbol') return Symbol('fixture'); if (v.tag === 'coerce') return {[Symbol.toPrimitive](hint) {s.events.push(['coerce', hint]); if (v.throw) throw new TypeError('coerce'); return v.value;}}; return v; }
function attempt(o, key, args = []) {try {return normal(o[key](...args));} catch (e) {return '#' + e.name;}}
function metadata(r) {
    const C = r.class === 'ParameterNode' ? ParameterNode : PropertyNode, n = new C(...r.args.map(v => decode(v))), p = C.prototype;
    return {own: Object.getOwnPropertyNames(n).map(k => {const d = Object.getOwnPropertyDescriptor(n, k); return [k, k === 'id' ? '#id' : normal(n[k]), d.writable, d.enumerable, d.configurable];}),
        methods: Object.getOwnPropertyNames(p).map(k => {const d = Object.getOwnPropertyDescriptor(p, k); return [k, enc(d.value), d.value.name, d.value.length, d.writable, d.enumerable, d.configurable];}),
        parent: Object.getPrototypeOf(p).constructor.name, staticParent: Object.getPrototypeOf(C).name, constructor: [C.name, C.length], type: n.type, abstractCall: attempt(p, 'constructor')};
}
function protocol(r) {
    const s = {events: [], r}, C = r.class === 'ParameterNode' ? ParameterNode : PropertyNode;
    const n = new C(r.nodeType, decode(r.name, s), decode(r.varying, s)); n.id = decode(r.id ?? 123, s);
    const variable = {name: 'variable', needsInterpolation: false}, data = {}, properties = {}, structure = {getMemberType(...a) {s.events.push(['member', ...a.map(normal)]); if (r.throwAt === 'member') throw new TypeError('member'); return 'vec2';}};
    const builder = {_fixtureTag: 'builder', getDataFromNode(...a) {s.events.push(['data', ...a.map(normal)]); return data;}, getNodeProperties(...a) {s.events.push(['properties', ...a.map(normal)]); return properties;},
        getOutputType(...a) {s.events.push(['output', ...a.map(normal)]); return 'vec4';},
        getVarFromNode(...a) {s.events.push(['variable', ...a.map(normal)]); if (r.throwAt === 'variable') throw new TypeError('variable'); return variable;},
        getVaryingFromNode(...a) {s.events.push(['varying', ...a.map(normal)]); if (r.throwAt === 'varying') throw new TypeError('varying'); return variable;},
        hasWriteUsage(...a) {s.events.push(['write', ...a.map(normal)]); if (r.throwAt === 'write') throw new TypeError('write'); return decode(r.write, s);},
        getPropertyName(...a) {s.events.push(['property', ...a.map(normal)]); if (r.throwAt === 'property') throw new TypeError('property'); return variable.name;},
        addLineFlowCode(...a) {s.events.push(['line', ...a.map(normal)]); if (r.throwAt === 'line') throw new TypeError('line');},
        getStructTypeNode(...a) {s.events.push(['struct', ...a.map(normal)]); if (r.throwAt === 'struct') throw new TypeError('struct'); return decode(r.structure, s) === null ? null : r.structure === 'undefined' ? undefined : structure;}
    };
    if (r.placeholder) n.placeholderNode = {build(...a) {s.events.push(['placeholder', ...a.map(normal)]); if (r.throwAt === 'placeholder') throw new TypeError('placeholder'); return 'snippet';}};
    if (r.observed) {
        function observe(o, key, name) {const value = o[key]; Object.defineProperty(o, key, {configurable: true, get() {s.events.push(['get', name, this === o]); if (r.throwAt === name) throw new TypeError(name); return value;}});}
        for (const key of ['getVarFromNode', 'getVaryingFromNode', 'getPropertyName', 'addLineFlowCode', 'getStructTypeNode', 'hasWriteUsage']) observe(builder, key, 'builder.' + key);
        for (const key of ['getNodeType', 'placeholderNode', 'name', 'varying']) observe(n, key, 'node.' + key);
    }
    const result = attempt(n, r.operation, [builder, decode(r.member, s)]);
    return {result, events: s.events, variable: normal(variable), data: normal(data)};
}
function superMethod(r) {
    const home = PropertyNode.prototype, prior = Object.getPrototypeOf(home), n = new PropertyNode('float', null), events = [];
    const replacement = Object.create(prior); Object.defineProperty(replacement, r.operation, {get() {events.push(['get', this === n]); return function(...args) {events.push(['call', this === n, args.length]); if (r.throw) throw new TypeError('super'); return r.value;};}});
    Object.setPrototypeOf(home, replacement); let result; try {result = attempt(n, r.operation, [{getOutputType() {events.push(['output']);return 'vec4';}}]);} finally {Object.setPrototypeOf(home, prior);} return {result, events};
}
function tsl(r) {
    const events = [], s = {events};
    let value = decode(r.value, s);
    if (r.value && typeof r.value === 'object') {
        if (r.value.tag === 'negativeZero') value = -0;
        if (r.value.tag === 'nan') value = NaN;
        if (r.value.tag === 'infinity') value = Infinity;
        if (r.value.tag === 'function') {value = () => {}; value.isFn = true;}
        if (r.value.tag === 'buffer') value = new ArrayBuffer(4);
        if (r.value.tag === 'node') value = new PropertyNode('float');
        if (r.value.tag === 'flags') {
            value = {};
            for (const flag of ['isNode', 'isVector2', 'isVector3', 'isVector4', 'isMatrix2', 'isMatrix3', 'isMatrix4', 'isColor']) Object.defineProperty(value, flag, {get() {
                events.push(['get', flag, this === value]); if (r.throwAt === flag) throw new TypeError('flag'); return r.value.flag === flag;
            }});
        }
        if (r.value.tag === 'typeFields') {
            value = {};
            for (const field of ['nodeType', 'convertTo']) Object.defineProperty(value, field, {get() {
                events.push(['get', field, this === value]); if (r.throwAt === field) throw new TypeError('field'); return decode(r.value[field], s);
            }});
        }
    }
    const results = [], raw = [];
    for (let i = 0; i < 2; i++) {
        try {
            const result = r.operation === 'getConstNodeType' ? getConstNodeType(value) : nodeObject(value, decode(r.altType, s)); raw.push(result);
            results.push(result?.isConstNode === true ? {constant: true, nodeType: enc(result.nodeType), value: normal(result.value), valueSame: result.value === value, isWeak: result.isWeak} : {sameValue: Object.is(result, value), value: enc(result)});
        } catch (e) {results.push('#' + e.name); raw.push(undefined);}
    }
    return {results, sameResult: Object.is(raw[0], raw[1]), events};
}
function tslIds(r) {return r.values.map(value => nodeObject(value).id - nodeObject(r.base).id);}
function run(r) {if (r.kind === 'metadata') return metadata(r); if (r.kind === 'super') return superMethod(r); if (r.kind === 'tsl') return tsl(r); if (r.kind === 'tslIds') return tslIds(r); return protocol(r);}
const cases = [];
for (const C of ['PropertyNode', 'ParameterNode']) for (const args of [[], ['float'], ['output', 'name'], ['vec3', null, true, 1], ['float', '', false, false]]) cases.push({kind: 'metadata', class: C, args});
for (const C of ['PropertyNode', 'ParameterNode']) for (const operation of ['getNodeType', 'getHash', 'customCacheKey', 'generate', 'getMemberType']) for (const name of [null, '', 'property', 0, {tag: 'coerce', value: 'named'}])
    cases.push({kind: 'protocol', class: C, operation, nodeType: 'float', name, varying: false, write: false, structure: 'struct', member: 'member'});
for (const varying of [false, true, 1, 'true', null]) for (const write of [false, true, 0, null, {tag: 'undefined'}]) for (const observed of [false, true])
    cases.push({kind: 'protocol', class: 'PropertyNode', operation: 'generate', nodeType: 'output', name: 'property', varying, write, placeholder: true, observed});
for (const throwAt of ['variable', 'varying', 'write', 'property', 'line', 'placeholder', 'builder.getPropertyName', 'builder.addLineFlowCode', 'node.placeholderNode', 'node.getNodeType', 'node.name'])
    for (const varying of [false, true]) cases.push({kind: 'protocol', class: 'PropertyNode', operation: 'generate', nodeType: 'float', name: 'property', varying, write: false, placeholder: true, observed: true, throwAt});
for (const structure of [null, 'struct', 'undefined']) for (const throwAt of ['', 'struct', 'member', 'builder.getStructTypeNode']) cases.push({kind: 'protocol', class: 'ParameterNode', operation: 'getMemberType', nodeType: 'Struct', name: 'parameter', varying: false, structure, member: 'field', observed: true, throwAt});
for (const operation of ['getNodeType', 'getHash']) for (const value of ['output', 'float', 8]) for (const throws of [false, true]) cases.push({kind: 'super', operation, value, throw: throws});
for (const value of [null, {tag: 'undefined'}, false, true, '', 'float', 0, {tag: 'negativeZero'}, 1, -1, 3, 4, -3, 0.5, -0.5, 1 / 3, Math.PI, -Math.PI, 8.75, {tag: 'nan'}, {tag: 'infinity'}, {}, {tag: 'node'}, {tag: 'function'}, {tag: 'buffer'}])
    for (const altType of [null, 'int', false, {tag: 'undefined'}]) cases.push({kind: 'tsl', operation: 'nodeObject', value, altType});
for (const flag of ['isNode', 'isVector2', 'isVector3', 'isVector4', 'isMatrix2', 'isMatrix3', 'isMatrix4', 'isColor', 'none'])
    for (const throwAt of ['', flag]) cases.push({kind: 'tsl', operation: 'nodeObject', value: {tag: 'flags', flag}, altType: 'vec4', throwAt});
for (const nodeType of [undefined, null, false, '', 'float', 7]) for (const convertTo of [undefined, null, false, 'vec3', 4])
    cases.push({kind: 'tsl', operation: 'getConstNodeType', value: {tag: 'typeFields', nodeType: nodeType === undefined ? {tag: 'undefined'} : nodeType, convertTo: convertTo === undefined ? {tag: 'undefined'} : convertTo}});
for (const value of [null, {tag: 'undefined'}, 'float', '', 1, false]) cases.push({kind: 'tsl', operation: 'getConstNodeType', value});
for (const throwAt of ['nodeType', 'convertTo']) cases.push({kind: 'tsl', operation: 'getConstNodeType', value: {tag: 'typeFields', nodeType: null, convertTo: 'vec4'}, throwAt});
cases.push({kind: 'tslIds', base: 0.5, values: [1.5, -0.5]});
const cachedFloats = [0.5, 1.5, 1 / 3, 1e-6, 1e6, Math.PI, Math.PI * 2, 1 / Math.PI, 2 / Math.PI, 1 / (Math.PI * 2), Math.PI / 2];
cases.push({kind: 'tslIds', base: 0, values: [0, 1, 2, 3, -1, -2, ...cachedFloats, ...cachedFloats.map(value => -value)]});
cases.push({kind: 'tslIds', base: false, values: [true, 0, 1, 2, 3, -1, -2, 0.5, -0.5]});
const oldWarn = console.warn, oldError = console.error; let expected; try {console.warn = console.error = () => {}; expected = cases.map(run);} finally {console.warn = oldWarn; console.error = oldError;}
if (output) {fs.mkdirSync(output, {recursive: true}); for (const [key, value] of [['input', cases], ['expected', expected]]) fs.writeFileSync(path.join(output, 'property-' + key + '.json'), JSON.stringify(value, null, 2));}
if (binary) {const actual = JSON.parse(execFileSync(binary, [], {input: JSON.stringify(cases), encoding: 'utf8', maxBuffer: 128 * 1024 * 1024})); assert.equal(actual.length, expected.length); const differences = []; for (let i = 0; i < cases.length; i++) try {assert.deepEqual(actual[i], expected[i]);} catch {differences.push({index: i, request: cases[i], expected: expected[i], actual: actual[i]});} if (output) for (const [key, value] of [['actual', actual], ['differences', differences]]) fs.writeFileSync(path.join(output, 'property-' + key + '.json'), JSON.stringify(value, null, 2)); assert.equal(differences.length, 0, JSON.stringify(differences[0]));}
console.log(`Property/Parameter ${binary ? 'parity' : 'real upstream oracle'}: ${cases.length} scenarios`);
