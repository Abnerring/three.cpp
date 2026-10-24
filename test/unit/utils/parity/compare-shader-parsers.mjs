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
const load = async p => (await import(pathToFileURL(path.join(upstream, 'src', p + '.js')))).default;
const [NodeParser, GLSLNodeParser, GLSLNodeFunction, WGSLNodeParser, WGSLNodeFunction, FunctionNode] = await Promise.all([
    'nodes/core/NodeParser', 'nodes/parsers/GLSLNodeParser', 'nodes/parsers/GLSLNodeFunction',
    'renderers/webgpu/nodes/WGSLNodeParser', 'renderers/webgpu/nodes/WGSLNodeFunction', 'nodes/code/FunctionNode'
].map(load));
const constructors = { NodeParser, GLSLNodeParser, GLSLNodeFunction, WGSLNodeParser, WGSLNodeFunction };
const enc = v => typeof v === 'number' && (Number.isNaN(v) || !Number.isFinite(v) || Object.is(v, -0))
    ? { number: Number.isNaN(v) ? 'NaN' : Object.is(v, -0) ? '-0' : v < 0 ? '-Infinity' : 'Infinity' }
    : v === undefined ? '#undefined' : typeof v === 'symbol' ? '#symbol' : typeof v === 'function' ? '#function' : v !== null && typeof v === 'object' ? '#object' : v;
function normal(v) {
    if (v === null || typeof v !== 'object') return enc(v);
    if (Array.isArray(v)) return Array.from(v, normal);
    const out = {}; for (const key of Object.keys(v)) Object.defineProperty(out, key, { value: normal(v[key]), enumerable: true, writable: true, configurable: true }); return out;
}
function attempt(f) { try { return normal(f()); } catch (e) { return e.name === 'Error' ? { error: e.name, message: e.message } : { error: e.name }; } }
function decode(v, events, label) {
    if (v === null || typeof v !== 'object') return v;
    if (v.tag === 'undefined') return undefined;
    if (v.tag === 'symbol') return Symbol('fixture');
    if (v.tag === 'coerce') return { [Symbol.toPrimitive](hint) { events.push(['coerce', label, hint]); if (v.throw) throw new TypeError('coerce'); return v.value; } };
    return v;
}
// Keep the trim receiver in an explicit closure without changing the value model.
function valueFor(v, events, label) {
    if (v?.tag !== 'trim') return decode(v, events, label);
    const value = {}; Object.defineProperty(value, 'trim', { get() { events.push(['getTrim', label]); return function () { events.push(['trim', label, this === value]); return decode(v.value, events, label + ':trim'); }; } }); return value;
}
function descriptor(object, key) {
    const d = Object.getOwnPropertyDescriptor(object, key), v = d.value;
    return [key, 'value' in d ? 'data' : 'accessor', enc(v), !!d.writable, !!d.enumerable, !!d.configurable,
        typeof v === 'function' ? v.name : enc(d.get), typeof v === 'function' ? v.length : enc(d.set)];
}
const valid = lang => lang === 'GLSL' ? 'highp vec3 fixture(const in vec3 a, out float[4] b) { return a; }' : 'fn fixture(a: vec3f, b: ptr<function, f32>) -> vec3f { return a; }';
const fnClass = lang => constructors[lang + 'NodeFunction'];
const parserClass = lang => constructors[lang + 'NodeParser'];
function metadata(r) {
    const C = constructors[r.class], n = r.class.endsWith('Function') ? new C(valid(r.class.startsWith('GLSL') ? 'GLSL' : 'WGSL')) : new C();
    return { own: Object.getOwnPropertyNames(n).map(k => [k, normal(n[k]), ...['writable', 'enumerable', 'configurable'].map(p => !!Object.getOwnPropertyDescriptor(n, k)[p])]),
        constructor: Object.getOwnPropertyNames(C).map(k => descriptor(C, k)), prototype: Object.getOwnPropertyNames(C.prototype).map(k => descriptor(C.prototype, k)),
        parent: Object.getPrototypeOf(C.prototype).constructor.name, staticParent: Object.getPrototypeOf(C).name,
        call: attempt(() => C()), methodConstruct: attempt(() => new C.prototype[r.class.endsWith('Parser') ? 'parseFunction' : 'getCode']()) };
}
function parse(r) {
    const events = [], sourceValue = decode(r.source, events, 'source'); let source = sourceValue;
    if (r.mock) {
        source = {}; const method = r.lang === 'GLSL' ? 'trim' : 'replace';
        Object.defineProperty(source, method, { get() { events.push(['sourceGet', method]); if (r.throwAt === 'get') throw new TypeError(); return function (...args) { events.push(['sourceCall', method, this === source, args.map(enc)]); if (r.throwAt === 'call') throw new TypeError(); return sourceValue; }; } });
    }
    const result = attempt(() => {
        const f = r.parser ? new (parserClass(r.lang))().parseFunction(source) : new (fnClass(r.lang))(source);
        return { fields: normal(f), codes: [attempt(() => f.getCode()), attempt(() => f.getCode(undefined)), attempt(() => f.getCode('renamed')), attempt(() => f.getCode(null)), attempt(() => f.getCode(Symbol('name')))],
            functionPrototype: Object.getPrototypeOf(f) === fnClass(r.lang).prototype,
            inputPrototypes: f.inputs.map(x => Object.getPrototypeOf(x).constructor.name) };
    });
    return { result, events };
}
function getCode(r) {
    const events = [], f = new (fnClass(r.lang))(valid(r.lang));
    const keys = r.lang === 'GLSL' ? ['name', 'blockCode', 'type', 'inputsCode', 'headerCode', 'precision'] : ['name', 'outputType', 'inputsCode', 'blockCode'];
    for (const key of keys) {
        const original = f[key], supplied = r.values && Object.hasOwn(r.values, key) ? r.values[key] : original; let reads = 0;
        Object.defineProperty(f, key, { configurable: true, enumerable: true, get() { events.push(['get', key]); if (r.throwAt === key) throw new TypeError();
            const v = supplied?.tag === 'sequence' ? supplied.values[Math.min(reads++, supplied.values.length - 1)] : supplied; return valueFor(v, events, key); } });
    }
    const result = attempt(() => Object.hasOwn(r, 'rename') ? f.getCode(valueFor(r.rename, events, 'rename')) : f.getCode());
    return { result, events };
}
function superConstructor(r) {
    const C = r.parser ? parserClass(r.lang) : fnClass(r.lang), saved = Object.getPrototypeOf(C), events = [];
    class Replacement { constructor(...args) { events.push(['super', normal(args), new.target === C]); if (r.throw) throw new TypeError(); this.replaced = true; } }
    try { Object.setPrototypeOf(C, Replacement); return { result: attempt(() => normal(new C(r.source || valid(r.lang), 2, 3))), events }; }
    finally { Object.setPrototypeOf(C, saved); }
}
function integration(r) {
    const parser = new (parserClass(r.lang))(), f = new FunctionNode(r.source), events = [], data = {}, record = { name: 'generated', code: '' };
    const original = parser.parseFunction; parser.parseFunction = function (source) { events.push(['parse', source]); return original.call(this, source); };
    const builder = { parser, getDataFromNode(n) { events.push(['data', n === f]); return data; }, getCodeFromNode(n, type) { events.push(['code', n === f, type]); return record; },
        registerDeclaration(code) { events.push(['register', code === record, code.name]); if (r.rename) code.name = 'reservedName'; }, getPropertyName(code) { events.push(['property', code === record]); return code.name; },
        format(code, type, output) { events.push(['format', code, type, output]); return code + ':' + type + ':' + output; } };
    const first = f.getNodeFunction(builder), second = f.getNodeFunction(builder);
    return { cached: first === second, fields: normal(first), inputsSame: f.getInputs(builder) === first.inputs, generated: attempt(() => f.generate(builder, r.output)),
        code: normal(record), registered: data.declarationRegistered === true, events };
}
function regexpProtocol(r) {
    const C = fnClass(r.lang), saved = Object.getOwnPropertyDescriptor(RegExp.prototype, 'exec'), events = []; let calls = 0, tokens = 0;
    Object.defineProperty(RegExp.prototype, 'exec', { configurable: true, get() {
        const regexp = this; events.push(['execGet', regexp.source, regexp.lastIndex]);
        return function (source) {
            events.push(['execCall', this === regexp, source, regexp.lastIndex]); if (++calls === r.throwCall) throw new TypeError('exec');
            if (r.tokens && regexp.source === '[a-z_0-9]+') return tokens < r.tokens.length ? [decode(r.tokens[tokens++], events, 'token')] : null;
            const result = saved.value.call(this, source);
            if (r.observeCaptures && result && (regexp.source.startsWith('^\\s*(highp') || regexp.source.startsWith('^[fn]') || regexp.source.startsWith('[a-z_0-9]') || regexp.source.startsWith('([a-z_0-9]'))) {
                for (let i = 0; i < result.length; ++i) {
                    const value = result[i]; Object.defineProperty(result, i, { configurable: true, enumerable: true, get() { events.push(['capture', regexp.source, i]); return value; } });
                }
            }
            return result;
        };
    } });
    let result; try { result = attempt(() => { const f = new C(valid(r.lang)); const out = { fields: normal(f), code: f.getCode('renamed') }; if (r.tokens) out.counts = f.inputs.map(i => Object.is(i.count, -0) ? '#-0' : normal(i.count)); return out; }); }
    finally { Object.defineProperty(RegExp.prototype, 'exec', saved); }
    return { result, events };
}
function prototypeSetters(r) {
    const C = r.parser ? parserClass(r.lang) : fnClass(r.lang), events = [];
    const saved = r.keys.map(key => [key, Object.getOwnPropertyDescriptor(Object.prototype, key)]);
    for (const key of r.keys) Object.defineProperty(Object.prototype, key, { configurable: true, set(value) {
        events.push(['set', key, enc(value)]); if (r.throw) throw new TypeError('prototype setter');
        Object.defineProperty(this, key, { value, writable: true, enumerable: true, configurable: true });
    } });
    let result;
    try { result = attempt(() => r.parser ? new C().parseFunction(valid(r.lang)) : new C(valid(r.lang))); }
    finally { for (const [key, descriptor] of saved) { if (descriptor) Object.defineProperty(Object.prototype, key, descriptor); else delete Object.prototype[key]; } }
    return { result, events };
}
function pushProtocol(r) {
    const C = r.parser ? parserClass(r.lang) : fnClass(r.lang), events = [];
    const savedPush = Object.getOwnPropertyDescriptor(Array.prototype, 'push'), savedExec = Object.getOwnPropertyDescriptor(RegExp.prototype, 'exec');
    Object.defineProperty(Array.prototype, 'push', { configurable: true, get() {
        savedPush.value.call(events, ['pushGet']); if (r.throwAt === 'push') throw new TypeError('push getter'); return savedPush.value;
    } });
    Object.defineProperty(RegExp.prototype, 'exec', { configurable: true, writable: true, value(...args) {
        const result = savedExec.value.apply(this, args);
        if (result && this.source.startsWith('([a-z_0-9]')) for (let i = 1; i <= 2; ++i) {
            const value = result[i]; Object.defineProperty(result, i, { configurable: true, enumerable: true, get() {
                savedPush.value.call(events, ['capture', i]); if (r.throwAt === 'capture') throw new TypeError('capture getter'); return value;
            } });
        }
        return result;
    } });
    let result;
    try { result = attempt(() => r.parser ? new C().parseFunction(valid(r.lang)) : new C(valid(r.lang))); }
    finally { Object.defineProperty(Array.prototype, 'push', savedPush); Object.defineProperty(RegExp.prototype, 'exec', savedExec); }
    return { result, events };
}
function run(r) {
    if (r.kind === 'metadata') return metadata(r); if (r.kind === 'parse') return parse(r); if (r.kind === 'getCode') return getCode(r);
    if (r.kind === 'super') return superConstructor(r); if (r.kind === 'integration') return integration(r); if (r.kind === 'regexp') return regexpProtocol(r); if (r.kind === 'prototypeSetters') return prototypeSetters(r); if (r.kind === 'pushProtocol') return pushProtocol(r); throw new Error('unknown fixture');
}
const cases = [];
for (const name of Object.keys(constructors)) cases.push({ kind: 'metadata', class: name });
const glsl = [
    'float f(){}', 'float f()', 'float ()', 'float f() ', 'float f()\n', 'void main(){ }', 'FLOAT Name(IN VEC3 p) {return p;}', 'highp float f() {return 1.;}',
    'mediump vec4 shade(const in vec3 p, out float[4] a, inout int n) { }', 'lowp void f(){}', 'highpfloat f(){}', 'highp f(){}',
    'float f(float a[3]){}', 'float f(float[3] a){}', 'float f(float 0xff a){}', 'float f(float 1e3 a){}', 'float f(float 0x a){}', 'float f(float 0008 a){}',
    'float f(float 9x a){}', 'float f(float a,float b){}', 'float f(const float a){}', 'float f(in float a){}', 'float f(out float a){}', 'float f(inout float a){}',
    'float f(highp float a){}', 'float f(void){}', 'float f(const){}', 'float f(in){}', 'float f(float){}', 'float f(float 3){}', 'float f(, ,){}',
    '/* leading */ // line\nfloat f() { /*body*/ }', '\ufeff\u2028/*x*/\r\nfloat f(){}', '/* unclosed float f(){}', '// no newline float f(){}',
    'float helper(){}\n#pragma main\n// main\nvec3 f(vec3 x){return x;}', '#pragma mainfloat f(){}', '#pragma main\nfloat f(){}\n#pragma main second',
    'not a function', '', 'float f(', 'float f(float a) /* retained */', 'float f(float a, vec3 b)\n{\r\nreturn a;\n}',
    'struct Foo { float x; }; Foo f(){}', 'float f(float _123, float 123abc named){}', 'float f(float 99999999999999999999999999999999999999 a){}',
    'float f(float 0xffffffffffffffffffffffff a){}', 'float f(float ' + '9'.repeat(400) + ' a){}'
];
const wgsl = [
    'fn f() -> f32 {}', 'fn f() -> f32', 'fn f() {}', 'fn f() -> void {}', 'fn () -> f32 {}', 'f32 f(){}', 'f() -> f32{}',
    'fn f(a: vec3<f32>, b: ptr<function, f32>) -> vec3f { return a; }', '// leading\n/*x*/fn f(x: f32) -> f32 {return x;}\n ',
    '\ufeff\u2028fn f() -> f32 {}', 'fn f(a: array<f32, 4>) -> array<f32, 4> {}', 'fn f(a: ptr<function, array<f32, 4>>) -> ptr<function, f32> {}',
    'fn f(@location(0) a: vec3f) -> f32 {}', 'fn f(a : Custom) -> Custom {}', 'fn f(a: vec3< f32 >) -> vec3< f32 > {}',
    'fn f(a: PTR<function,f32>) -> F32 {}', 'fn f(a: constructor, b: __proto__, c: toString) -> constructor {}',
    'nnfff f() f32 {}', 'fnf()-->>f32{}', 'fn f(a:f32) - > f32 {}', 'fn f() -> f16 {}', 'fn f() -> mat2x3<f32> {}',
    '', 'fn', 'fn f(', 'fn f() ->', '/*unclosed fn f() -> f32 {}', '// no newline fn f()-> f32{}'
];
const wgslTypes = ['f32', 'i32', 'u32', 'bool', 'sampler', 'sampler_comparison', 'texture_1d<f32>', 'texture_2d<f32>', 'texture_2d_array<f32>',
    'texture_multisampled_2d<f32>', 'texture_depth_2d', 'texture_depth_2d_array', 'texture_depth_multisampled_2d', 'texture_depth_cube', 'texture_depth_cube_array',
    'texture_3d<f32>', 'texture_cube<f32>', 'texture_cube_array<f32>', 'texture_storage_1d<rgba8unorm, write>', 'texture_storage_2d<rgba8unorm, write>',
    'texture_storage_2d_array<rgba8unorm, write>', 'texture_storage_3d<rgba8unorm, write>', 'ptr<storage, f32, read_write>', 'Custom', 'array<f32, 4>', 'f16'];
for (const n of [2, 3, 4]) { for (const t of ['f32', 'i32', 'u32', 'bool']) wgslTypes.push(`vec${n}<${t}>`); for (const t of ['f','i','u','b']) wgslTypes.push(`vec${n}${t}`); wgslTypes.push(`mat${n}x${n}<f32>`, `mat${n}x${n}f`); }
for (const type of wgslTypes) wgsl.push(`fn typeTest(value: ${type}) -> ${type} { return value; }`);
for (const [lang, sources] of [['GLSL', glsl], ['WGSL', wgsl]]) for (const source of sources) for (const parser of [false, true]) cases.push({ kind: 'parse', lang, source, parser });
for (const lang of ['GLSL', 'WGSL']) {
    for (const observeCaptures of [false, true]) for (const throwCall of [0, 1, 2, 3, 4, 5, 6]) cases.push({ kind: 'regexp', lang, observeCaptures, throwCall });
    for (const source of [null, true, 4, {}, { tag: 'undefined' }, { tag: 'symbol' }, { tag: 'coerce', value: valid(lang) }]) cases.push({ kind: 'parse', lang, source });
    for (const throwAt of ['', 'get', 'call']) cases.push({ kind: 'parse', lang, source: valid(lang), mock: true, throwAt });
    for (const rename of [undefined, '', null, 3, { tag: 'undefined' }, { tag: 'symbol' }, { tag: 'coerce', value: 'newName' }, { tag: 'coerce', value: 'newName', throw: true }]) {
        const r = { kind: 'getCode', lang }; if (rename !== undefined) r.rename = rename; cases.push(r);
    }
    const fields = lang === 'GLSL' ? ['name', 'blockCode', 'type', 'inputsCode', 'headerCode', 'precision'] : ['name', 'outputType', 'inputsCode', 'blockCode'];
    for (const key of fields) {
        cases.push({ kind: 'getCode', lang, throwAt: key });
        for (const value of [null, 0, '', { tag: 'undefined' }, { tag: 'symbol' }, { tag: 'coerce', value: key + '-value' }, { tag: 'coerce', value: key + '-value', throw: true }]) cases.push({ kind: 'getCode', lang, values: { [key]: value } });
    }
    for (const value of ['x', null, { tag: 'symbol' }, { tag: 'coerce', value: 'INPUTS' }, { tag: 'coerce', value: 'INPUTS', throw: true }]) cases.push({ kind: 'getCode', lang, values: { inputsCode: { tag: 'trim', value } } });
    for (const parser of [false, true]) for (const throws of [false, true]) cases.push({ kind: 'super', lang, parser, throw: throws });
    for (const output of ['property', 'float']) for (const rename of [false, true]) cases.push({ kind: 'integration', lang, source: valid(lang), output, rename });
}
for (const values of [{ blockCode: '' }, { precision: null }, { type: { tag: 'coerce', value: 'TYPE' }, name: { tag: 'coerce', value: 'NAME' }, inputsCode: { tag: 'trim', value: { tag: 'coerce', value: 'ARGS' } }, headerCode: { tag: 'coerce', value: 'HEADER' }, precision: { tag: 'coerce', value: 'PRECISION' }, blockCode: { tag: 'coerce', value: 'BLOCK' } }]) cases.push({ kind: 'getCode', lang: 'GLSL', values });
cases.push({ kind: 'getCode', lang: 'WGSL', values: { outputType: { tag: 'sequence', values: ['f32', 'void'] } } });
cases.push({ kind: 'super', lang: 'GLSL', source: 'invalid shader' }, { kind: 'super', lang: 'WGSL', source: 'invalid shader' });
for (const count of [' -12tail', '\ufeff+0xFf', '-0', '0b10', '0o12', '+0012', '-0x10', 'Infinity', '', null, true, 5.9,
    { tag: 'coerce', value: ' -12x' }, { tag: 'coerce', value: '-0xF' }, { tag: 'coerce', value: '2', throw: true }, { tag: 'symbol' }]) cases.push({ kind: 'regexp', lang: 'GLSL', tokens: ['float', count, 'value'] });
for (const keys of [['name'], ['type'], ['name', 'type']]) for (const parser of [false, true]) for (const throws of [false, true]) cases.push({ kind: 'prototypeSetters', lang: 'WGSL', keys, parser, throw: throws });
for (const parser of [false, true]) for (const throwAt of ['', 'push', 'capture']) cases.push({ kind: 'pushProtocol', lang: 'WGSL', parser, throwAt });
const oldWarn = console.warn; let expected;
try { console.warn = () => {}; expected = cases.map(run); } finally { console.warn = oldWarn; }
if (outputDirectory) { fs.mkdirSync(outputDirectory, { recursive: true }); for (const [name, value] of [['input', cases], ['expected', expected]]) fs.writeFileSync(path.join(outputDirectory, 'shader-parsers-' + name + '.json'), JSON.stringify(value, null, 2)); }
if (binary) {
    const actual = JSON.parse(execFileSync(binary, [], { input: JSON.stringify(cases), encoding: 'utf8', maxBuffer: 256 * 1024 * 1024 })); assert.equal(actual.length, expected.length);
    const differences = []; for (let i = 0; i < cases.length; ++i) try { assert.deepEqual(actual[i], expected[i]); } catch { differences.push({ index: i, request: cases[i], expected: expected[i], actual: actual[i] }); }
    if (outputDirectory) for (const [name, value] of [['actual', actual], ['differences', differences]]) fs.writeFileSync(path.join(outputDirectory, 'shader-parsers-' + name + '.json'), JSON.stringify(value, null, 2));
    assert.equal(differences.length, 0, `${differences.length} shader parser differences; first ${JSON.stringify(differences[0])}`);
}
console.log(`Shader parsers ${binary ? 'parity' : 'real upstream oracle'}: ${cases.length} scenarios`);
