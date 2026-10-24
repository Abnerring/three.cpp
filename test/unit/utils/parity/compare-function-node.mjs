import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream = path.resolve(process.argv[2]);
const binary = process.argv[3] && process.argv[3] !== '--oracle' ? path.resolve(process.argv[3]) : undefined;
const outputDirectory = process.argv[4] && path.resolve(process.argv[4]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const load = async name => (await import(pathToFileURL(path.join(upstream, 'src/nodes/' + name + '.js')))).default;
const [CodeNode, FunctionNode, NodeFunction, NodeFunctionInput, NodeBuilder] = await Promise.all(['code/CodeNode', 'code/FunctionNode', 'core/NodeFunction', 'core/NodeFunctionInput', 'core/NodeBuilder'].map(load));
const constructors = {CodeNode, FunctionNode, NodeFunction, NodeFunctionInput};
const enc = v => v === undefined ? '#undefined' : typeof v === 'symbol' ? '#symbol' : typeof v === 'function' ? '#function' : v !== null && typeof v === 'object' ? '#object' : v;
function normal(v, seen = []) {
    if (v === null || typeof v !== 'object') return enc(v);
    if (v.isNode === true) return '@' + v.name;
    if (v._fixtureTag) return '#' + v._fixtureTag;
    if (seen.includes(v)) return '#cycle'; seen = [...seen, v];
    if (Array.isArray(v)) return Array.from(v, x => normal(x, seen));
    const out = {}; for (const key of Object.keys(v)) Object.defineProperty(out, key, {value: normal(v[key], seen), writable: true, enumerable: true, configurable: true}); return out;
}
function decode(v, s) {
    if (v === null || typeof v !== 'object') return v;
    if (v.tag === 'undefined') return undefined;
    if (v.tag === 'symbol') return Symbol('fixture');
    if (v.tag === 'coerce') return {[Symbol.toPrimitive](hint) { s.events.push(['coerce', hint]); if (v.throw) throw new TypeError('coerce'); return v.value; }};
    return v;
}
function node(ctor, name, ...args) { const n = new ctor(...args); n.name = name; n.id = {main: 100, other: 101, include: 102, a: 103, b: 104}[name]; n._uuid = 'uuid-' + name; return n; }
function attempt(object, key, args = []) { try { return normal(object[key](...args)); } catch (e) { return '#' + e.name; } }
function descriptor(object, key) {
    const d = Object.getOwnPropertyDescriptor(object, key);
    return [key, Object.hasOwn(d, 'value') ? 'data' : 'accessor', enc(d.value), !!d.writable, !!d.enumerable, !!d.configurable,
        d.value && typeof d.value === 'function' ? d.value.name : enc(d.get), d.value && typeof d.value === 'function' ? d.value.length : enc(d.set)];
}
function metadata(r) {
    const C = constructors[r.class], args = (r.args || []).map(x => decode(x));
    const n = new C(...args), p = C.prototype, out = {};
    out.own = Object.getOwnPropertyNames(n).map(key => [key, key === 'id' ? '#id' : normal(n[key]), ...descriptor(n, key).slice(3, 6)]);
    out.constructor = Object.getOwnPropertyNames(C).map(key => descriptor(C, key));
    out.prototype = Object.getOwnPropertyNames(p).map(key => descriptor(p, key));
    out.parent = Object.getPrototypeOf(p).constructor.name; out.staticParent = Object.getPrototypeOf(C).name;
    out.call = attempt(p, 'constructor');
    if (r.class === 'CodeNode' || r.class === 'FunctionNode') {
        out.includesDistinct = n.includes !== new C().includes; out.type = n.type; out.cacheable = n.isCacheable();
        out.includeIdentity = n.getIncludes() === n.includes;
        const replacement = {}; out.setIncludes = n.setIncludes(replacement) === n && n.includes === replacement;
    } else if (r.class === 'NodeFunction') out.abstract = attempt(n, 'getCode', ['function']);
    return out;
}
function observe(s, object, key, label) {
    const value = object[key]; Object.defineProperty(object, key, {configurable: true, enumerable: true, get() {
        s.events.push(['get', label, this === object]); if (s.r.throwAt === label) throw new TypeError(label); return value;
    }});
}
function protocol(r) {
    const s = {r, events: [], parses: 0, registrations: 0}, n = s.n = node(r.class === 'CodeNode' ? CodeNode : FunctionNode, 'main', 'SOURCE', [], 'glsl');
    const data = {}, record = {name: 'automatic', type: 'float', code: ''};
    const inputs = [new NodeFunctionInput('float', 'value')];
    const parsed = new NodeFunction('float', inputs, decode(r.name, s), 'highp');
    parsed.getCode = function(...args) { s.events.push(['getCode', this === parsed, ...args.map(v => normal(v))]); if (r.throwAt === 'getCode') throw new TypeError('getCode'); return Object.hasOwn(r, 'source') ? decode(r.source, s) : 'function-source'; };
    const parser = {parseFunction(...args) {
        s.events.push(['parseFunction', this === parser, ...args.map(v => normal(v))]); s.parses++;
        if (r.reenterParse && s.parses === 1) s.events.push(['parseNested', attempt(n, 'getNodeFunction', [builder])]);
        if (r.throwAt === 'parseFunction') throw new TypeError('parseFunction');
        return Object.hasOwn(r, 'parseResult') ? decode(r.parseResult, s) : parsed;
    }};
    const builder = {
        _fixtureTag: 'builder', parser,
        getDataFromNode(...args) { s.events.push(['getDataFromNode', this === builder, ...args.map(v => normal(v))]); if (r.throwAt === 'getDataFromNode') throw new TypeError('getDataFromNode'); return data; },
        getCodeFromNode(...args) { s.events.push(['getCodeFromNode', this === builder, ...args.map(v => normal(v))]); if (r.throwAt === 'getCodeFromNode') throw new TypeError('getCodeFromNode'); return record; },
        registerDeclaration(...args) {
            s.events.push(['registerDeclaration', this === builder, ...args.map(v => normal(v))]); s.registrations++;
            if (r.reenterRegister && s.registrations === 1) s.events.push(['registerNested', attempt(n, 'generate', [builder, 'property'])]);
            if (r.throwAt === 'registerDeclaration') throw new TypeError('registerDeclaration'); if (r.rename) record.name = 'renamed';
        },
        getPropertyName(...args) { s.events.push(['getPropertyName', this === builder, ...args.map(v => normal(v))]); if (r.throwAt === 'getPropertyName') throw new TypeError('getPropertyName'); return record.name; },
        format(...args) { s.events.push(['format', this === builder, ...args.map(v => normal(v))]); if (r.throwAt === 'format') throw new TypeError('format'); return args[0] + '|' + args[1] + '>' + args[2]; },
        getStructTypeNode(...args) { s.events.push(['getStructTypeNode', this === builder, ...args.map(v => normal(v))]); return {getMemberType(...a) {s.events.push(['getMemberType', ...a.map(v => normal(v))]); return 'member:' + a[1];}}; }
    };
    if (Object.hasOwn(r, 'cached')) data.nodeFunction = decode(r.cached, s);
    if (Object.hasOwn(r, 'registered')) data.declarationRegistered = decode(r.registered, s);
    const includes = [0, 1].map(index => ({build(...args) { s.events.push(['include', index, ...args.map(v => normal(v))]); if (r.throwAt === 'include' + index) throw new TypeError('include'); }}));
    if (r.iterable) n.includes = {[Symbol.iterator]() { let index = 0; return {next() {return {value: includes[index++], done: index > includes.length};}, return() {s.events.push(['closeIncludes']); return {done: true};}}; }};
    else n.includes = includes;
    if (r.observed) {
        for (const key of ['getDataFromNode', 'getCodeFromNode', 'registerDeclaration', 'getPropertyName', 'format', 'parser']) observe(s, builder, key, 'builder.' + key);
        observe(s, parser, 'parseFunction', 'parser.parseFunction');
        for (const key of ['getIncludes', 'getNodeType', 'getNodeFunction', 'code']) observe(s, n, key, 'node.' + key);
    }
    const results = [];
    for (let i = 0; i < 2; i++) results.push(attempt(n, r.operation, [builder, decode(r.output, s)]));
    return {results, events: s.events, data: normal(data), code: normal(record), parses: s.parses, registrations: s.registrations};
}
function serialization(r) {
    const C = constructors[r.class], n = node(C, 'main', 'SOURCE', [], 'wgsl'), a = node(CodeNode, 'a', 'A'), b = node(CodeNode, 'b', 'B');
    n.includes = [a, b];
    const data = {meta: {nodes: {}, textures: {}, images: {}}};
    if (r.operation === 'deserialize') { data.inputNodes = {includes: ['uuid-b', 'uuid-a']}; data.meta.nodes = {'uuid-a': a, 'uuid-b': b}; data.code = 'RESTORED'; data.language = 'glsl'; }
    const result = attempt(n, r.operation, [data]);
    return {result, data: normal(data), fields: [normal(n.code), normal(n.language), normal(n.includes)]};
}
function superMethod(r) {
    const C = constructors[r.class], n = node(C, 'main', 'SOURCE'), home = C.prototype, saved = Object.getPrototypeOf(home), events = [];
    const replacement = Object.create(saved);
    Object.defineProperty(replacement, r.operation, {get() { events.push(['superGet', this === n]); return function(...args) { events.push(['superCall', this === n, args.length]); if (r.throw) throw new TypeError('super'); }; }});
    Object.setPrototypeOf(home, replacement);
    let result;
    try {
        if (r.class === 'CodeNode') result = attempt(n, r.operation, [{}]);
        else {
            n.getNodeFunction = () => ({name: '', type: 'float', getCode: () => 'source'});
            const code = {}; result = attempt(n, 'generate', [{getCodeFromNode: () => code, getPropertyName: () => 'property'}, 'property']);
        }
    } finally { Object.setPrototypeOf(home, saved); }
    return {result, events};
}
function integration(r) {
    const events = [], parser = {parseFunction(code) { events.push(['parse', code]); const f = new NodeFunction('float', [], r.name); f.getCode = name => 'float ' + name + '(){ return 1.0; }'; return f; }};
    const renderer = {backend: {}, debug: {diagnostics: {keywords: r.reserved}}};
    const builder = new NodeBuilder(null, renderer, parser); builder.shaderStage = r.stage;
    builder.isReservedKeyword = name => name === r.name;
    const include = node(CodeNode, 'include', 'float included;');
    const nodes = [node(FunctionNode, 'main', 'FUNCTION-A', [include], 'glsl'), node(FunctionNode, 'other', 'FUNCTION-B', [include], 'glsl')];
    const results = [];
    for (const stage of ['setup', 'analyze', 'generate']) { builder.setBuildStage(stage); for (const n of nodes) results.push(attempt(n, 'build', [builder, r.output])); }
    return {results, events, codes: normal(builder.codes), declarations: Object.keys(builder.declarations[r.stage] || {}),
        source: attempt(builder, 'getCodes', [r.stage]), chaining: builder.chaining.length, buildStage: builder.buildStage};
}
function run(r) {
    if (r.kind === 'metadata') return metadata(r); if (r.kind === 'protocol') return protocol(r); if (r.kind === 'serialization') return serialization(r);
    if (r.kind === 'super') return superMethod(r); if (r.kind === 'integration') return integration(r); throw new TypeError('unknown function scenario');
}
const cases = [];
for (const name of Object.keys(constructors)) for (const args of [[], [{tag: 'undefined'}, {tag: 'undefined'}], [null, null, null, null, null], ['float', [], 'name', 'precision', true]]) cases.push({kind: 'metadata', class: name, args});
for (const operation of ['getNodeFunction', 'generateNodeType', 'getInputs', 'getMemberType', 'generate'])
    for (const observed of [false, true]) for (const name of ['', 'function', null, {tag: 'undefined'}]) for (const output of ['property', 'vec3'])
        cases.push({kind: 'protocol', class: 'FunctionNode', operation, observed, name, output});
for (const operation of ['getNodeFunction', 'generate']) for (const cached of [null, false, 0, {tag: 'undefined'}]) cases.push({kind: 'protocol', class: 'FunctionNode', operation, name: 'function', output: 'property', cached});
for (const registered of [true, false, 1, 'true', null]) for (const rename of [false, true]) cases.push({kind: 'protocol', class: 'FunctionNode', operation: 'generate', name: 'function', output: 'property', registered, rename});
for (const source of [0, null, false, {tag: 'undefined'}, {tag: 'symbol'}, {tag: 'coerce', value: 'COERCED'}, {tag: 'coerce', value: 'COERCED', throw: true}]) cases.push({kind: 'protocol', class: 'FunctionNode', operation: 'generate', name: 'function', output: 'property', source});
for (const reenterParse of [false, true]) for (const reenterRegister of [false, true]) cases.push({kind: 'protocol', class: 'FunctionNode', operation: 'generate', name: 'function', output: 'property', reenterParse, reenterRegister});
for (const throwAt of ['include0', 'include1', 'getDataFromNode', 'parseFunction', 'getCodeFromNode', 'registerDeclaration', 'getPropertyName', 'getCode', 'format', 'builder.getCodeFromNode', 'parser.parseFunction', 'node.code'])
    for (const iterable of [false, true]) cases.push({kind: 'protocol', class: 'FunctionNode', operation: 'generate', observed: true, name: 'function', output: 'vec3', throwAt, iterable});
for (const iterable of [false, true]) for (const throwAt of ['', 'include0', 'include1', 'getCodeFromNode']) cases.push({kind: 'protocol', class: 'CodeNode', operation: 'generate', name: 'unused', output: 'property', iterable, throwAt, observed: true});
for (const name of ['CodeNode', 'FunctionNode']) for (const operation of ['serialize', 'deserialize']) cases.push({kind: 'serialization', class: name, operation});
for (const operation of ['serialize', 'deserialize']) for (const throws of [false, true]) cases.push({kind: 'super', class: 'CodeNode', operation, throw: throws});
for (const throws of [false, true]) cases.push({kind: 'super', class: 'FunctionNode', operation: 'generate', throw: throws});
for (const stage of ['vertex', 'fragment', 'compute']) for (const reserved of [false, true]) for (const name of ['', 'function']) for (const output of ['property', 'float']) cases.push({kind: 'integration', stage, reserved, name, output});
const oldWarn = console.warn, oldError = console.error;
let expected;
try {console.warn = () => {}; console.error = () => {}; expected = cases.map(run);} finally {console.warn = oldWarn; console.error = oldError;}
if (outputDirectory) {fs.mkdirSync(outputDirectory, {recursive: true}); for (const [name, value] of [['input', cases], ['expected', expected]]) fs.writeFileSync(path.join(outputDirectory, 'function-' + name + '.json'), JSON.stringify(value, null, 2));}
if (binary) {
    const actual = JSON.parse(execFileSync(binary, [], {input: JSON.stringify(cases), encoding: 'utf8', maxBuffer: 256 * 1024 * 1024})); assert.equal(actual.length, expected.length);
    const differences = []; for (let i = 0; i < cases.length; i++) try {assert.deepEqual(actual[i], expected[i]);} catch {differences.push({index: i, request: cases[i], expected: expected[i], actual: actual[i]});}
    if (outputDirectory) for (const [name, value] of [['actual', actual], ['differences', differences]]) fs.writeFileSync(path.join(outputDirectory, 'function-' + name + '.json'), JSON.stringify(value, null, 2));
    assert.equal(differences.length, 0, `${differences.length} function differences; first ${JSON.stringify(differences[0])}`);
}
console.log(`Function classes ${binary ? 'parity' : 'real upstream oracle'}: ${cases.length} scenarios`);
assert.equal(typeof global.gc, 'function', 'Use node --expose-gc');
let live, weak;
function setupGC() { const code = new CodeNode(), fn = new FunctionNode(), input = new NodeFunctionInput('float', 'v'), parsed = new NodeFunction('float', [input]); code.includes = [fn]; fn.includes = [code]; fn.parsed = parsed; input.owner = code; live = code; weak = [code, fn, parsed, input].map(v => new WeakRef(v)); }
async function jobs() {await new Promise(setImmediate); await new Promise(setImmediate); global.gc(); await new Promise(setImmediate); global.gc();}
setupGC(); await jobs(); const gc = [weak.map(v => !!v.deref())]; live = undefined; await jobs(); gc.push(weak.map(v => !!v.deref()));
if (outputDirectory) fs.writeFileSync(path.join(outputDirectory, 'function-gc-expected.json'), JSON.stringify(gc, null, 2));
if (binary) {const actual = JSON.parse(execFileSync(binary, ['gc'], {encoding: 'utf8'})); if (outputDirectory) fs.writeFileSync(path.join(outputDirectory, 'function-gc-actual.json'), JSON.stringify(actual, null, 2)); assert.deepEqual(actual, gc);}
console.log(`Function classes GC: ${gc.length} snapshots`);
