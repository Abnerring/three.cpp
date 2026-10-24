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
const { default: BindGroup } = await import(pathToFileURL(path.join(upstream, 'src/renderers/common/BindGroup.js')));
const { default: FunctionNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/code/FunctionNode.js')));
const { default: ParameterNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/core/ParameterNode.js')));
const RendererNodeUniforms = await import(pathToFileURL(path.join(upstream, 'src/renderers/common/nodes/NodeUniform.js')));
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
function bindGroupMetadata() {
    const first = new BindGroup(), second = new BindGroup(), bindings = [], explicitGroup = new BindGroup(null, bindings);
    const normalized = snapshot(first); normalized.id.value = 0;
    expected.push({ kind: 'bindGroupMetadata', constructor: snapshot(BindGroup), prototype: snapshot(BindGroup.prototype), default: normalized,
        idGap: second.id - first.id, separateArrays: first.bindings !== second.bindings, explicitName: encode(explicitGroup.name),
        bindingsIdentity: explicitGroup.bindings === bindings, instancePrototype: Object.getPrototypeOf(first) === BindGroup.prototype, call: attempt(() => BindGroup()) });
}
function bindGroupShared(mode) {
    const events = [], group = {}, otherGroup = {}, bindings = {}, manager = {}, otherManager = {}, first = {}, second = {}, third = {}, sharedResult = {}, renderer = {}, self = {};
    const accessor = (object, key, get) => Object.defineProperty(object, key, { get, configurable: true, enumerable: true });
    accessor(group, 'shared', () => { events.push('shared'); return mode & 1 ? 'shared' : false; });
    for (const [index, value] of [first, second, third].entries()) {
        accessor(value, 'groupNode', () => { events.push(`group${index}`); if (index === 2 && (mode & 16)) throw RangeError('third group'); return index === 1 && (mode & 2) ? otherGroup : group; });
        accessor(bindings, String(index), () => { events.push(`binding${index}`); return value; });
    }
    accessor(bindings, 'length', () => { events.push('length'); return 3; });
    for (const value of [manager, otherManager]) accessor(value, 'getSharedBindGroup', function () {
        events.push('method.get'); const receiver = this;
        return function getSharedBindGroup(name, values) { events.push(['method.call', this === receiver, name, values === bindings]); return sharedResult; };
    });
    let reads = 0;
    accessor(renderer, '_bindings', () => { const count = reads++; events.push(`_bindings${count}`); if (mode & 4) return undefined; return count !== 0 && (mode & 8) ? otherManager : manager; });
    accessor(self, 'renderer', () => { events.push('renderer'); return renderer; });
    let result; const outcome = attempt(() => result = NodeBuilder.prototype._getBindGroup.call(self, 'frame', bindings));
    expected.push({ kind: 'bindGroupShared', mode, events, outcome, shared: result === sharedResult,
        local: result !== undefined && Object.getPrototypeOf(result) === BindGroup.prototype,
        bindingsIdentity: result !== undefined && result !== sharedResult && result.bindings === bindings });
}
function bindGroupBasic() {
    const b = builder(), group = { shared: false, order: 7 }, other = { shared: false, order: -1 }, a = { label: 'a', groupNode: group }, c = { label: 'c', groupNode: group }, z = { label: 'z', groupNode: other };
    b.getBindGroupArray('frame', 'fragment').push(a, a); b.getBindGroupArray('frame', 'vertex').push(c, a); b.getBindGroupArray('frame', 'compute').push(c); b.getBindGroupArray('object', 'vertex').push(z);
    const groups = b.getBindings(), rows = groups.map(value => ({ name: encode(value.name), labels: value.bindings.map(item => encode(item.label)), prototype: Object.getPrototypeOf(value) === BindGroup.prototype }));
    const cached = b.getBindings() === groups; b.sortBindingGroups();
    expected.push({ kind: 'bindGroupBasic', groups: rows, cached, sorted: [encode(groups[0].name), encode(groups[1].name)], indexes: b.bindingsIndexes });
}
function bindGroupsEnumeration(mode) {
    const self = builder(), events = [], inherited = {}, fragment = Object.create(inherited), vertex = {}, bindings = self.bindings;
    const values = label => [{ label }]; fragment[2] = values('two'); fragment.later = values('later'); inherited.inherited = values('inherited');
    Object.defineProperty(fragment, 'shadow', { value: values('hidden') }); inherited.shadow = values('shadow');
    Object.defineProperty(fragment, '1', { configurable: true, enumerable: true, get() {
        events.push('first.get'); if (mode & 1) delete fragment.later;
        if (mode & 2) { fragment.added = values('added'); inherited.addedPrototype = values('addedPrototype'); }
        if (mode & 4) vertex.nextStage = values('nextStage');
        if (mode & 8) self.bindings = { fragment: { replaced: values('replaced') } };
        return values('one');
    } });
    bindings.fragment = fragment; bindings.vertex = vertex;
    Object.defineProperty(self, '_getBindGroup', { configurable: true, enumerable: true, get() {
        events.push('factory.get'); return function _getBindGroup(name, group) { events.push(['factory.call', name, this === self]); return { name, labels: group.map(value => value.label) }; };
    } });
    let result; const outcome = attempt(() => result = NodeBuilder.prototype.getBindings.call(self));
    expected.push({ kind: 'bindGroupsEnumeration', mode, outcome, events, groups: result ?? null, cached: result === self.bindGroups, bindingsReplaced: self.bindings !== bindings });
}
function bindGroupsFailure(mode) {
    const self = builder(), events = [], bindings = self.bindings; let count = 0;
    bindings.fragment = { first: [1, 1, 2], second: [3] };
    const original = Object.getOwnPropertyDescriptor(Array.prototype, 'includes');
    if (mode < 3) Object.defineProperty(Array.prototype, 'includes', { configurable: true, writable: true, value: function includes(value) {
        events.push(['includes', value]); return mode === 0 ? false : mode === 1 ? 0 : undefined;
    } });
    self._getBindGroup = function _getBindGroup(name, group) { const index = count++; events.push(['factory', name, group]); if (mode === 3 && index === 1) throw RangeError('factory'); return { name, values: group }; };
    if (mode === 4) Object.defineProperty(self, 'bindGroups', { configurable: true, enumerable: true, get() { return null; }, set(value) { events.push('bindGroups.set'); throw RangeError('set'); } });
    if (mode >= 5 && mode <= 7) { const special = {}; special[mode === 5 ? 'constructor' : mode === 6 ? 'toString' : '__proto__'] = [1]; bindings.fragment = special; }
    if (mode >= 8) self.bindGroups = mode === 8 ? undefined : mode === 9 ? false : mode === 10 ? 0 : 'cached';
    let result; const outcome = attempt(() => result = NodeBuilder.prototype.getBindings.call(self));
    if (mode < 3) Object.defineProperty(Array.prototype, 'includes', original);
    expected.push({ kind: 'bindGroupsFailure', mode, events, outcome, groups: result !== null && typeof result === 'object' ? result : encode(result), cache: encode(self.bindGroups) });
}
function bindGroupsSort(mode) {
    const self = builder(), events = [], groups = [], indexes = {}, replacement = {}; let conversions = 0;
    for (let i = 0; i < 8; ++i) {
        const label = `group${i}`, order = { [Symbol.toPrimitive](hint) { events.push(['convert', label, hint]); const count = conversions++; if (mode === 2 && count === 1) throw RangeError('convert'); return (i * 5 + 3) % 6; } };
        const groupNode = {}; Object.defineProperty(groupNode, 'order', { configurable: true, enumerable: true, get() { events.push(['order.get', label]); if (mode === 1) throw RangeError('order'); return order; } });
        const group = { bindings: [{ groupNode }], testLabel: label };
        Object.defineProperty(group, 'name', { configurable: true, enumerable: true, get() { events.push(['name.get', label]); if (mode === 6) self.bindingsIndexes = replacement; return label; } });
        const entry = { group: -1 };
        if (mode === 5) Object.defineProperty(entry, 'group', { configurable: true, enumerable: true, get() { return -1; }, set(value) { events.push(['group.set', label, value]); if (value === 1) throw RangeError('index'); } });
        indexes[label] = entry; replacement[label] = { group: -2 }; groups.push(group);
    }
    self.bindingsIndexes = indexes;
    Object.defineProperty(self, 'getBindings', { configurable: true, enumerable: true, get() {
        events.push('getBindings.get'); if (mode === 4) throw RangeError('getBindings'); return function getBindings() { events.push('getBindings.call'); return groups; };
    } });
    const nativeSort = groups.sort;
    Object.defineProperty(groups, 'sort', { configurable: true, enumerable: true, get() {
        events.push('sort.get'); if (mode === 3) throw RangeError('sort'); return function sort(compare) { events.push(['sort.call', this === groups, compare.length]); if (mode === 7) return []; return nativeSort.call(this, compare); };
    } });
    const outcome = attempt(() => NodeBuilder.prototype.sortBindingGroups.call(self));
    expected.push({ kind: 'bindGroupsSort', mode, outcome, events, names: groups.map(value => value.testLabel),
        indexes: Array.from({ length: 8 }, (_, i) => encode(indexes[`group${i}`].group)), replacement: Array.from({ length: 8 }, (_, i) => encode(replacement[`group${i}`].group)) });
}
function bindGroupsIterator(mode) {
    const self = builder(), events = []; let count = 0;
    const iterator = { next() {
        events[events.length] = 'next'; const index = count++;
        if (mode === 0) throw RangeError('next'); if (mode === 4) return 1;
        const result = { done: index !== 0 };
        if (mode === 5) Object.defineProperty(result, 'value', { configurable: true, enumerable: true, get() { events[events.length] = 'value.get'; throw RangeError('value'); } }); else result.value = 1;
        return result;
    } };
    Object.defineProperty(iterator, 'return', { configurable: true, enumerable: true, get() {
        events[events.length] = 'return.get'; if (mode === 3) throw TypeError('return getter'); return function () { events[events.length] = 'return.call'; return {}; };
    } });
    self.bindings.fragment = { frame: { [Symbol.iterator]() { return iterator; } } };
    self._getBindGroup = function (name, values) { return { name, values }; };
    const original = Object.getOwnPropertyDescriptor(Array.prototype, 'includes');
    if (mode === 1 || mode === 2 || mode === 3) Object.defineProperty(Array.prototype, 'includes', { configurable: true, get() {
        events[events.length] = 'includes.get'; if (mode !== 2) throw RangeError('includes getter');
        return function () { events[events.length] = 'includes.call'; throw RangeError('includes call'); };
    } });
    const outcome = attempt(() => NodeBuilder.prototype.getBindings.call(self));
    if (mode === 1 || mode === 2 || mode === 3) Object.defineProperty(Array.prototype, 'includes', original);
    expected.push({ kind: 'bindGroupsIterator', mode, outcome, events, cache: encode(self.bindGroups) });
}
function functionCacheBasic() {
    const backend = {}, otherBackend = {}, shader = {}, otherShader = {}, calls = [], first = builder(), second = builder();
    for (const self of [first, second]) {
        self.renderer.backend = backend; self.buildFunctionCode = function (input) { calls.push([Object.getPrototypeOf(this.currentFunctionNode) === FunctionNode.prototype, input === shader]); return 'float functionCode() { return 2.0; }'; };
    }
    const a = first.buildFunctionNode(shader), again = second.buildFunctionNode(shader), other = second.buildFunctionNode(otherShader);
    second.renderer.backend = otherBackend; const differentBackend = second.buildFunctionNode(shader);
    expected.push({ kind: 'functionCacheBasic', same: a === again, otherShader: a !== other, otherBackend: a !== differentBackend,
        prototype: Object.getPrototypeOf(a) === FunctionNode.prototype, code: encode(a.code), calls, restored: first.currentFunctionNode === null && second.currentFunctionNode === null });
}
function functionCacheProtocol(mode) {
    const self = {}, events = [], backend = mode & 4 ? 7 : {}, shader = mode & 8 ? 9 : {}, previous = {}, renderer = {}; let current = previous;
    const accessor = (target, name, get, set) => Object.defineProperty(target, name, { get, set, configurable: true, enumerable: true });
    accessor(self, 'renderer', () => { events.push('renderer'); return renderer; }); accessor(renderer, 'backend', () => { events.push('backend'); return backend; });
    accessor(self, 'currentFunctionNode', () => { events.push('current.get'); return current; }, value => { events.push(['current.set', value === previous ? 'previous' : 'function']); current = value; });
    accessor(self, 'buildFunctionCode', () => {
        events.push('build.get'); if (mode & 1) throw RangeError('build getter');
        return function (input) { events.push(['build.call', this === self, input === shader, Object.getPrototypeOf(current) === FunctionNode.prototype]); if (mode & 2) throw RangeError('build call'); return 'compiled'; };
    });
    let first, second; const outcome = attempt(() => first = NodeBuilder.prototype.buildFunctionNode.call(self, shader)); const partial = current;
    const repeated = attempt(() => second = NodeBuilder.prototype.buildFunctionNode.call(self, shader));
    expected.push({ kind: 'functionCacheProtocol', mode, outcome, repeated, events, cached: first !== undefined && first === second,
        partialPrevious: partial === previous, currentPrevious: current === previous, partialChanged: partial !== current, code: first === undefined ? null : encode(first.code) });
}
function rendererNodeUniform(index) {
    const names = ['float', 'int', 'uint', 'vec2', 'ivec2', 'uvec2', 'vec3', 'ivec3', 'uvec3', 'vec4', 'ivec4', 'uvec4', 'color', 'mat2', 'mat3', 'mat4', 'bool', 'bvec2', 'texture', 'unknown'];
    const type = names[index], self = builder(), source = { name: 'uniform', value: 7, type }; let result;
    const outcome = attempt(() => result = self.getNodeUniform(source, type)); const row = { kind: 'rendererNodeUniform', index, type, outcome };
    if (result !== undefined) {
        row.snapshot = snapshot(result); row.nodeIdentity = result.nodeUniform === source; row.firstValue = encode(result.getValue());
        source.value = 19; row.liveValue = encode(result.getValue()); row.liveType = encode(result.getType());
        const className = index < 3 ? 'NumberNodeUniform' : index < 6 ? 'Vector2NodeUniform' : index < 9 ? 'Vector3NodeUniform' : index < 12 ? 'Vector4NodeUniform' : index === 12 ? 'ColorNodeUniform' : index === 13 ? 'Matrix2NodeUniform' : index === 14 ? 'Matrix3NodeUniform' : 'Matrix4NodeUniform';
        row.prototype = Object.getPrototypeOf(result) === RendererNodeUniforms[className].prototype;
    }
    expected.push(row);
}
function flowShaderProtocol(mode) {
    const events = [], self = {}, shader = {}, layout = {}, iterable = {}, iterator = {}, input = {}, callNode = {}, result = {};
    const state = {events, mode, shader, layout, currentLayout: layout, iterator, input, callNode, result, count: 0, inputs: undefined, names: 0};
    Object.defineProperty(shader, 'layout', {enumerable: true, configurable: true,
        get() { events.push('layout.get'); if (mode === 16) throw RangeError('layout'); return state.currentLayout; },
        set(value) { events.push(['layout.set', value === null ? 'null' : 'original']); if ((mode === 6 && value === null) || (mode === 7 && value !== null)) throw RangeError('layout set'); state.currentLayout = value; }});
    Object.defineProperty(layout, 'inputs', {enumerable: true, configurable: true, get() { events.push('inputs.get'); if (mode === 17) throw RangeError('inputs'); return iterable; }});
    Object.defineProperty(layout, 'type', {enumerable: true, configurable: true, get() { events.push('type.get'); if (mode === 4) throw RangeError('type'); return 'vec3'; }});
    Object.defineProperty(iterable, Symbol.iterator, {enumerable: true, configurable: true, get() {
        events.push('iterator.get'); if (mode === 15) throw RangeError('iterator'); return function () { events.push('iterator.call'); return iterator; };
    }});
    iterator.next = function next() { events.push('next'); if (mode === 10) throw RangeError('next'); return {done: state.count++ !== 0, value: input}; };
    Object.defineProperty(iterator, 'return', {enumerable: true, configurable: true, get() {
        events.push('return.get'); if (mode === 11) throw TypeError('return'); return function () { events.push('return.call'); return {}; };
    }});
    Object.defineProperty(input, 'name', {enumerable: true, configurable: true, get() {
        const count = state.names++; events.push(['name.get', count]);
        if (mode === 8 || mode === 11 || (mode === 12 && count === 1)) throw RangeError('name');
        return mode === 13 ? undefined : mode === 14 ? null : 'weight';
    }});
    Object.defineProperty(input, 'type', {enumerable: true, configurable: true, get() { events.push('input.type'); if (mode === 9) throw RangeError('input type'); return 'float'; }});
    Object.defineProperty(shader, 'call', {enumerable: true, configurable: true, get() {
        events.push('call.get'); if (mode === 1) throw RangeError('call getter'); if (mode === 18) return 0;
        return function (inputs) { events.push(['call.call', this === shader, state.currentLayout === null]); state.inputs = inputs; if (mode === 2) throw RangeError('call'); return callNode; };
    }});
    Object.defineProperty(self, 'flowStagesNode', {enumerable: true, configurable: true, get() {
        events.push('flow.get'); if (mode === 3) throw RangeError('flow getter'); if (mode === 19) return 0;
        return function (node, type) { events.push(['flow.call', node === callNode, type]); if (mode === 5) throw RangeError('flow'); return result; };
    }});
    let value; const outcome = attempt(() => value = NodeBuilder.prototype.flowShaderNode.call(self, shader));
    const inputs = state.inputs ? Object.getOwnPropertyNames(state.inputs).map(key => {
        const n = state.inputs[key]; return {key, name: encode(n.name), nodeType: encode(n.nodeType), parameter: Object.getPrototypeOf(n) === ParameterNode.prototype, global: encode(n.global), varying: encode(n.varying), placeholder: encode(n.placeholderNode)};
    }) : [];
    expected.push({kind: 'flowShaderProtocol', mode, outcome, events, inputs, restored: state.currentLayout === layout, nullLayout: state.currentLayout === null, resultIdentity: value === result});
}
function flowShaderIterator(mode) {
    const self = {}, shader = {layout: {inputs: [], type: 'float'}}, events = [], state = {mode, events, rows: []};
    shader.call = function (inputs) {
        const borrowed = {2: 2, 1: 1, tail: 4}; borrowed[Symbol('skip')] = 99;
        Object.defineProperty(borrowed, 'first', {enumerable: true, configurable: true, get() { events.push('first.get'); delete borrowed.tail; borrowed.added = 5; return 3; }});
        delete borrowed.tail; borrowed.tail = 4;
        const original = Object.getOwnPropertyDescriptor(Object, 'values');
        if (mode !== 0) Object.defineProperty(Object, 'values', {configurable: true, get() {
            events.push('values.get'); if (mode === 1) throw RangeError('values getter'); if (mode === 5) return 0;
            return function (value) {
                events.push(['values.call', this === Object, value === borrowed]); if (mode === 2) throw RangeError('values'); if (mode === 6) return null;
                const values = {};
                for (let i = 0; i < 4; ++i) Object.defineProperty(values, i, {enumerable: true, configurable: true, get() {
                    events.push(['value.get', i]); if (mode === 3 && i === 1) throw RangeError('value'); return 9 - i;
                }});
                Object.defineProperty(values, 'length', {enumerable: true, configurable: true, get() { events.push('length.get'); if (mode === 7) throw RangeError('length'); return 1; }});
                return values;
            };
        }});
        const method = inputs[Symbol.iterator]; let iterator;
        state.outcome = attempt(() => iterator = method.call(borrowed));
        Object.defineProperty(Object, 'values', original);
        const rows = [];
        if (iterator !== undefined) for (let i = 0; i < 4; ++i) {
            let row; const outcome = attempt(() => row = iterator.next.call(i === 1 ? null : iterator)); rows.push({outcome, row: row === undefined ? encode(row) : JSON.parse(JSON.stringify(row))});
        }
        state.protocol = {methodName: method.name, methodLength: method.length, symbolEnumerable: Object.getOwnPropertyDescriptor(inputs, Symbol.iterator).enumerable,
            iteratorKeys: iterator ? [Object.getOwnPropertyNames(iterator)[0] ?? ''] : [], iterable: iterator !== undefined && iterator[Symbol.iterator] !== undefined};
        state.rows = rows; return {};
    };
    self.flowStagesNode = function () { return 'flow'; };
    const flow = attempt(() => NodeBuilder.prototype.flowShaderNode.call(self, shader));
    expected.push({kind: 'flowShaderIterator', mode, flow, outcome: state.outcome, protocol: state.protocol, rows: state.rows, events});
}
metadata(); typesAndFormat(); dataAndUsage(); declarationAndFlow(); nodeBuild(); dynamic();
for (let mode = 0; mode < 32; ++mode) dataProtocol(mode);
for (let mode = 0; mode < 4; ++mode) callOrder(mode);
bindGroupMetadata(); bindGroupBasic();
for (let mode = 0; mode < 32; ++mode) bindGroupShared(mode);
for (let mode = 0; mode < 16; ++mode) bindGroupsEnumeration(mode);
for (let mode = 0; mode < 12; ++mode) bindGroupsFailure(mode);
for (let mode = 0; mode < 8; ++mode) bindGroupsSort(mode);
for (let mode = 0; mode < 6; ++mode) bindGroupsIterator(mode);
functionCacheBasic();
for (let mode = 0; mode < 16; ++mode) functionCacheProtocol(mode);
for (let index = 0; index < 20; ++index) rendererNodeUniform(index);
for (let mode = 0; mode < 20; ++mode) flowShaderProtocol(mode);
for (let mode = 0; mode < 8; ++mode) flowShaderIterator(mode);
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
