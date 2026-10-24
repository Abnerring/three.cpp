import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [upstream, probe, output] = process.argv.slice(2);
if (!upstream || !probe) throw Error('compare-node-builder.mjs <pinned-three.js> <probe> [output-directory]');
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), '11738a9d55e502fb484cefffcea3a66dc44e9fe9');
// Import the actual upstream class and its actual dependencies, including TSL's
// StackNode, without transplanting or replacing its implementation.
const load = async name => (await import(pathToFileURL(path.join(upstream, `src/nodes/core/${name}.js`)))).default;
const [NodeBuilder, Node, NodeCache, NodeAttribute, NodeUniform, NodeVar, NodeVarying, NodeCode, StructType] =
    await Promise.all(['NodeBuilder', 'Node', 'NodeCache', 'NodeAttribute', 'NodeUniform', 'NodeVar', 'NodeVarying', 'NodeCode', 'StructType'].map(load));
const { setConsoleFunction } = await import(pathToFileURL(path.join(upstream, 'src/utils.js')));
setConsoleFunction(() => {});
const encode = value => value === undefined ? '#undefined' : typeof value === 'function' ? '#function' :
    typeof value === 'symbol' ? '#symbol' : value !== null && typeof value === 'object' ? '#object' :
    typeof value === 'number' && !Number.isFinite(value) ? String(value) : value;
const descriptor = d => ({ enumerable: d.enumerable, configurable: d.configurable,
    ...('value' in d ? { value: encode(d.value), writable: d.writable } : { get: typeof d.get === 'function', set: typeof d.set === 'function' }) });
const snapshot = value => Object.fromEntries(Object.getOwnPropertyNames(value).map(key => [key, descriptor(Object.getOwnPropertyDescriptor(value, key))]));
const attempt = fn => { try { return { value: encode(fn()) }; } catch (e) { return { error: e.name }; } };
const builder = () => { const b = new NodeBuilder(null, { debug: { diagnostics: { keywords: false } } }, null); b.shaderStage = 'fragment'; return b; };
const node = () => { const n = new Node('float'); n.getHash = function getHash() { return 'node-hash'; }; n.generate = function generate(builder) { return '4.0'; }; return n; };
const expected = [];
function metadata() {
    const b = builder(), proto = NodeBuilder.prototype;
    expected.push({ kind: 'metadata', constructor: snapshot(NodeBuilder), keys: Object.getOwnPropertyNames(b), instance: snapshot(b),
        methods: Object.fromEntries(['getDataFromNode', 'getNodeProperties', 'getVarFromNode', 'getVaryingFromNode', 'flowStagesNode', 'format', 'getTypeFromLength', 'getSubBuildProperty', 'build', 'generateConst'].map(method => [method, {
            name: encode(proto[method].name), length: encode(proto[method].length), construct: attempt(() => Reflect.construct(proto[method], []))
        }])), prototype: Object.getPrototypeOf(b) === proto, constructorIdentity: proto.constructor === NodeBuilder,
        globalCache: b.cache === b.globalCache, stack: encode(b.stack.isStackNode), call: attempt(() => NodeBuilder()) });
    for (const C of [NodeVar, NodeAttribute, NodeUniform, NodeVarying, NodeCode, StructType]) {
        const instance = new C('declaration', 'vec3', node());
        expected.push({ kind: 'dependency', name: C.name, instance: snapshot(instance), constructor: snapshot(C), prototype: snapshot(C.prototype),
            parent: Object.getPrototypeOf(C.prototype) === (C === NodeVarying ? NodeVar.prototype : Object.prototype) });
    }
}
const types = ['float', 'bool', 'int', 'uint', 'vec2', 'vec3', 'vec4', 'ivec2', 'ivec3', 'ivec4', 'uvec2', 'uvec3', 'uvec4', 'bvec2', 'bvec3', 'bvec4', 'mat2', 'mat3', 'mat4', 'color', 'texture', 'cubeTexture', 'depthTexture', 'texture3D', 'void', 'property', 'unknown', 'prefix_ivec3_suffix'];
function typesAndFormat() {
    const b = builder();
    for (const type of types) expected.push({ kind: 'types', type, ...Object.fromEntries([
        'getType', 'isScalar', 'isVector', 'isMatrix', 'isReference', 'getElementType', 'getComponentType', 'getVectorType', 'isInteger', 'getTypeLength', 'getVectorFromMatrix', 'getIntegerType', 'generateConst'
    ].map(method => [method, attempt(() => b[method](type))])) });
    for (const from of types) for (const to of types) expected.push({ kind: 'format', from, to, result: attempt(() => b.format('sample', from, to)) });
    for (const length of [0, 1, 2, 3, 4, 9, 16, 5]) for (const component of ['float', 'bool', 'int', 'uint', 'mat2'])
        expected.push({ kind: 'length', length, component, result: attempt(() => b.getTypeFromLength(length, component)) });
    for (const [index, C] of [Int8Array, Uint8Array, Int16Array, Uint16Array, Int32Array, Uint32Array, Float32Array, Float64Array, Uint8ClampedArray].entries())
        expected.push({ kind: 'arrayType', index, result: attempt(() => b.getTypeFromArray(new C(0))) });
}
function dataAndUsage() {
    const b = builder(), n = node(), global = node(); global.global = true;
    const base = b.getDataFromNode(n), globalData = b.getDataFromNode(global), properties = b.getNodeProperties(n);
    const subBuilds = new Set(['outer', 'inner']); b.getDataFromNode(n, 'any').subBuilds = subBuilds;
    b.addSubBuild('outer'); const outer = b.getDataFromNode(n); b.addSubBuild('inner'); const inner = b.getDataFromNode(n);
    const rows = [base !== outer, outer !== inner, outer.subBuilds === subBuilds, encode(b.getSubBuildProperty('variable', subBuilds))];
    rows.push(encode(b.removeSubBuild()), b.getDataFromNode(n) === outer, encode(b.removeSubBuild()), b.getDataFromNode(n) === base, b.getNodeProperties(n) === properties);
    rows.push(encode(b.increaseUsage(n)), encode(b.hasWriteUsage(n))); b.context.assign = true;
    rows.push(encode(b.increaseUsage(n)), encode(b.hasWriteUsage(n)), encode(base.readUsageCount), encode(base.writeUsageCount));
    const previous = b.cache, nested = b.getCacheFromNode(n); rows.push(nested.parent === previous); b.setCache(nested); rows.push(b.getDataFromNode(n) === base);
    b.setCache(new NodeCache()); rows.push(b.getDataFromNode(n) !== base, b.getDataFromNode(global) === globalData);
    expected.push({ kind: 'data', rows });
}
function declarationAndFlow() {
    const b = builder(), n = node(), other = node(), rows = [];
    const v = b.getVarFromNode(n, 'shared', 'float'); rows.push(b.getVarFromNode(n) === v, encode(b.getVarFromNode(other, 'shared', 'vec3').name), encode(b.getVars('fragment')));
    const u = b.getUniformFromNode(n, 'float'); u.value = 17; rows.push(encode(n.value), u.node === n);
    const varying = b.getVaryingFromNode(n); rows.push(encode(varying.isNodeVar), encode(varying.interpolationType));
    b.addLineFlowCode('line'); b.addLineFlowCode('already;  '); b.addLineFlowCode(''); b.addFlowTab(); b.addLineFlowCode('nested'); b.removeFlowTab();
    rows.push(encode(b.flow.code), encode(b.tab)); b.getCodeFromNode(n, 'code').code = 'functionCode'; rows.push(encode(b.getCodes('fragment')));
    const original = b.flow, cache = b.cache, stack = b.stack; b.buildStage = 'before'; const flow = b.flowStagesNode(n, 'vec3');
    rows.push(encode(flow.result), encode(flow.vars), b.flow === original, b.cache === cache, b.stack === stack, encode(b.buildStage));
    b.buildStage = 'generate'; const cross = b.flowNodeFromShaderStage('vertex', n, 'float', 'position'); rows.push(encode(cross.result), encode(b.flowCode.vertex), encode(b.shaderStage));
    expected.push({ kind: 'declarationFlow', rows });
}
function nodeBuild() {
    const b = builder(), n = node(), rows = [];
    b.buildStage = 'setup'; n.build(b, 'float'); b.buildStage = 'analyze'; n.build(b, 'float'); n.build(b, 'float'); b.buildStage = 'generate';
    rows.push(encode(n.build(b, 'float')), encode(b.flow.code), encode(n.build(b, 'vec3')), encode(b.flow.code), b.chaining.length);
    expected.push({ kind: 'nodeBuild', rows });
}
function dynamic() {
    const b = builder(), n = node(), events = [], data = {};
    b.getDataFromNode = function replacement(...args) { events.push(args.length > 1 ? args[1] : '#omitted'); return data; };
    b.increaseUsage(n); b.context.assign = true; b.increaseUsage(n); const properties = b.getNodeProperties(n);
    expected.push({ kind: 'dynamic', events, data, same: properties === data.properties });
    const clean = builder(), previous = clean.flow, flowNode = { build() { throw RangeError('failure'); } };
    const failure = attempt(() => clean.flowChildNode(flowNode));
    expected.push({ kind: 'abruptFlow', error: failure, restored: clean.flow === previous, code: encode(clean.flow.code) });
}
function dataProtocol(mode) {
    const self = {}, n = {}, cache = {}, events = [], state = { data: mode & 4 ? undefined : {} };
    const accessor = (object, key, get) => Object.defineProperty(object, key, { get, configurable: true });
    const stage = mode & 8 ? { [Symbol.toPrimitive](hint) { events.push(`key:${hint}`); return 'fragment'; } } : 'fragment';
    accessor(self, 'shaderStage', () => { events.push('shaderStage'); if (mode >= 28) throw RangeError('stage'); return stage; });
    accessor(n, 'isGlobal', () => { events.push('isGlobal.get'); return function isGlobal(builder) { events.push('isGlobal.call'); return !!(mode & 1); }; });
    for (const field of ['globalCache', 'cache']) accessor(self, field, () => { events.push(field); return cache; });
    accessor(cache, 'getData', () => { events.push('getData.get'); return function getData(node) { events.push('getData.call'); return state.data; }; });
    accessor(cache, 'setData', () => { events.push('setData.get'); return function setData(node, value) { events.push('setData.call'); state.data = value; }; });
    accessor(self, 'subBuildLayers', () => { events.push('subBuildLayers'); return []; });
    const args = [n]; if (mode & 2) args.push(stage); if (mode & 16) { if (args.length === 1) args.push(undefined); args.push(cache); }
    let result; const outcome = attempt(() => result = NodeBuilder.prototype.getDataFromNode.call(self, ...args));
    expected.push({ kind: 'dataProtocol', mode, events, outcome, dataIdentity: result !== undefined && result === state.data.fragment });
}
function callOrder(mode) {
    const b = builder(), n = node(), events = [];
    const accessor = (target, key, get) => Object.defineProperty(target, key, { get, configurable: true });
    const outer = () => { events.push('outer.get'); if (mode === 3) throw RangeError('outer'); return function outer(...args) { events.push('outer.call'); return args[0]; }; };
    const inner = result => () => { events.push('inner.get'); return function inner() { events.push('inner.call'); return result; }; };
    let result;
    if (mode === 0 || mode === 3) {
        accessor(b, 'setHashNode', outer); accessor(n, 'getHash', inner('hash'));
        result = attempt(() => NodeBuilder.prototype.addNode.call(b, n));
    } else if (mode === 1) {
        accessor(b, 'getTypeFromLength', outer); accessor(b, 'getTypeLength', inner(3));
        result = attempt(() => NodeBuilder.prototype.changeComponentType.call(b, 'vec3', 'int'));
    } else {
        accessor(b, 'getComponentType', outer); accessor(n, 'getNodeType', inner('float'));
        result = attempt(() => NodeBuilder.prototype.getPromotedComponentType.call(b, n));
    }
    expected.push({ kind: 'callOrder', mode, events, result });
}
metadata(); typesAndFormat(); dataAndUsage(); declarationAndFlow(); nodeBuild(); dynamic();
for (let mode = 0; mode < 32; ++mode) dataProtocol(mode);
for (let mode = 0; mode < 4; ++mode) callOrder(mode);
if (output) {
    mkdirSync(output, { recursive: true });
    writeFileSync(path.join(output, 'node-builder-upstream.jsonl'), expected.map(x => JSON.stringify(x)).join('\n') + '\n');
}
let nativeText;
try { nativeText = execFileSync(probe, [], { encoding: 'utf8', maxBuffer: 16 * 1024 * 1024 }); }
catch (error) {
    if (output) {
        writeFileSync(path.join(output, 'node-builder-native.jsonl'), error.stdout ?? '');
        writeFileSync(path.join(output, 'node-builder-native.err.log'), error.stderr ?? '');
    }
    throw Error(`NodeBuilder probe exited ${error.status}: ${String(error.stderr).trim()}`);
}
const actual = nativeText.trim().split(/\r?\n/).filter(Boolean).map(line => JSON.parse(line));
if (output) {
    writeFileSync(path.join(output, 'node-builder-native.jsonl'), actual.map(x => JSON.stringify(x)).join('\n') + '\n');
}
assert.equal(actual.length, expected.length, 'row count');
for (let i = 0; i < expected.length; ++i) assert.deepEqual(actual[i], expected[i], `row ${i}: ${expected[i].kind}`);
console.log(`NodeBuilder: ${expected.length} upstream rows match, including actual Node.build/StackNode/NodeCache integration.`);
