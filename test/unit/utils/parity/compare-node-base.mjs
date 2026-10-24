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
const {default: Node} = await import(pathToFileURL(path.join(upstream, 'src/nodes/core/Node.js')));
const encode = v => v === undefined ? '#undefined' : typeof v === 'symbol' ? '#symbol' : typeof v === 'function' ? '#function' :
    v !== null && typeof v === 'object' ? '#object' : Number.isNaN(v) ? '#NaN' : v === Infinity ? '#Infinity' : v === -Infinity ? '#-Infinity' : Object.is(v, -0) ? '#-0' : v;
function normal(v, ancestors = []) {
    if (v === null || typeof v !== 'object') return encode(v);
    if (v.isNode === true) return '@' + v.name;
    if (v._fixtureTag === 'builder') return '#builder';
    if (ancestors.includes(v)) return '#cycle';
    ancestors = [...ancestors, v];
    if (ancestors.length > 12) return '#depth';
    if (Array.isArray(v)) return Array.from(v, x => normal(x, ancestors));
    const out = {};
    for (const key of Object.keys(v)) Object.defineProperty(out, key, {value: normal(v[key], ancestors), writable: true, enumerable: true, configurable: true});
    return out;
}
function decode(v, s, label = '') {
    if (v === null || typeof v !== 'object') return v;
    if (v.tag === 'undefined') return undefined;
    if (v.tag === 'nan') return NaN;
    if (v.tag === 'symbol') return Symbol('fixture');
    if (v.tag === 'coerce') return {[Symbol.toPrimitive](hint) {
        s.events.push(['coerce', label, hint]);
        if (v.throw) throw new TypeError('coercion');
        return decode(v.value);
    }};
    return {};
}
function node(name = 'root', id = 100, args = []) {
    const n = new Node(...args); n.name = name; n.id = id; n._uuid = 'uuid-' + name; return n;
}
const state = request => ({request, events: [], root: node(), table: {}, counts: {}, generateCalls: 0});
function attempt(o, key, args = []) {
    try { return normal(o[key](...args)); }
    catch (e) { return '#' + e.name; }
}
function entry(s, n) { return s.table[n.name] ||= {data: {}, properties: {}}; }
function fn(s, name, arity = 0) {
    const result = function(...a) { return fixture(this, a, s, name); };
    Object.defineProperty(result, 'name', {value: name, configurable: true});
    Object.defineProperty(result, 'length', {value: arity, configurable: true});
    return result;
}
function fixture(self, a, s, name) {
    const r = s.request, b = s.builder, root = s.root;
    s.events.push([name, ...a.map(v => normal(v))]);
    if (r.throwAt === name) throw new TypeError(name);
    if (name === 'getDataFromNode') return entry(s, a[0]).data;
    if (name === 'getNodeProperties') return entry(s, a[0]).properties;
    if (name === 'getNodeFromHash') return s.shared;
    if (name === 'getBuildStage') return b.buildStage;
    if (name === 'setBuildStage') { b.buildStage = a[0]; return; }
    if (name === 'addChain') { s.chain.push(a[0].name); return; }
    if (name === 'removeChain') { s.chain.pop(); return; }
    if (name === 'increaseUsage') {
        const d = entry(s, a[0]).data, count = d.usageCount === undefined ? 1 : Number(d.usageCount) + 1;
        d.usageCount = count; return Object.hasOwn(r, 'usageResult') ? decode(r.usageResult) : count;
    }
    if (name === 'getElementType') return 'element:' + String(a[0]);
    if (name === 'getVectorType') return a[0];
    if (name === 'isReference') return Object.hasOwn(r, 'referenceResult') ? decode(r.referenceResult) : a[0] === 'reference';
    if (name === 'format') return String(a[0]) + '|' + String(a[1]) + '>' + String(a[2]);
    if (name === 'generateConst') return 'zero<' + String(a[0]) + '>';
    if (name === 'getVarFromNode') return {type: a[2], name: 'cacheVar'};
    if (name === 'getPropertyName') return a[0].name;
    if (name === 'generateLetStatement' || name === 'generateVarStatement')
        return (name === 'generateLetStatement' ? 'let<' : 'var<') + String(a[0]) + '> ' + String(a[1]) + '[' + String(a[2]) + ']';
    if (name === 'addLineFlowCode') { b.flow.code += String(a[0]) + '\n'; return; }
    if (name === 'setup') {
        if (r.reenterSetup) s.events.push(['setupNested', attempt(root, 'build', [b])]);
        return s.outputNode;
    }
    if (name === 'generate') {
        const count = ++s.generateCalls;
        if (r.reenterGenerate && count === 1) s.events.push(['generateNested', attempt(root, 'build', [b, 'float'])]);
        if (r.emitFlow) b.flow.code += 'emitted;';
        return Object.hasOwn(r, 'snippet') ? decode(r.snippet) : 'code';
    }
    if (name === 'isCacheable') return decode(r.cacheable);
    if (name === 'generateNodeType') return decode(r.typeResult);
    if (name === 'outputBuild') return 'out-code';
    if (name === 'beforeBuild') { s.events.push(['beforeCleared', root._beforeNodes === null]); return; }
    if (name === 'overrideCallback') {
        if (r.reenter) s.events.push(['sharedNested', attempt(root, 'getShared', [b])]);
        return s.overrideNode;
    }
    if (name === 'overrideNode') return r.falsyOverride ? false : s.overrideNode;
    if (name === 'boundUpdate' || name === 'boundReference') {
        s.events.push(['boundReceiver', self === root, a[0] === s.argument]); return s.argument;
    }
    if (name === 'listener') {
        s.events.push(['eventReceiver', self === root, a[0].target === root, a[0].type]);
        s.lastEvent = a[0]; if (r.removeListener) root.removeEventListener('dispose', s.listener); return;
    }
    if (name === 'childBuild') return 'child-code';
}
function builder(s) {
    const b = {context: {}, buildStage: 'setup', shaderStage: 'fragment', flow: {code: ''}, flowBlock: null};
    Object.defineProperty(b, '_fixtureTag', {value: 'builder', writable: true, configurable: true});
    s.builder = b; s.chain = [];
    for (const name of ['getDataFromNode', 'getNodeProperties', 'getNodeFromHash', 'getBuildStage', 'setBuildStage', 'addChain', 'removeChain', 'addNode', 'addSequentialNode', 'increaseUsage', 'getElementType', 'getVectorType', 'isReference', 'format', 'generateConst', 'getVarFromNode', 'getPropertyName', 'generateLetStatement', 'generateVarStatement', 'addLineFlowCode']) b[name] = fn(s, name);
    return b;
}
const finish = (s, result) => ({result, events: s.events});
function metadata(r) {
    const s = state(r), n = s.root, ctor = Node, p = Node.prototype;
    const out = {own: Object.getOwnPropertyNames(n), descriptors: Object.getOwnPropertyNames(n).map(key => {
        const d = Object.getOwnPropertyDescriptor(n, key); return [key, encode(n[key]), !!d.writable, !!d.enumerable, !!d.configurable];
    }), methods: Object.getOwnPropertyNames(p).map(key => {
        const d = Object.getOwnPropertyDescriptor(p, key), v = p[key];
        return [key, Object.hasOwn(d, 'value') ? 'data' : 'accessor', typeof v === 'function' ? v.name : encode(v),
            typeof v === 'function' ? v.length : '#undefined', !!d.writable, !!d.enumerable, !!d.configurable,
            typeof d.get === 'function', typeof d.set === 'function'];
    }), staticType: ctor.type, capture: ctor.captureStackTrace, constructorName: ctor.name, constructorLength: ctor.length,
    callConstructor: attempt(p, 'constructor')};
    const a = new Node(), b = new Node(); out.idStep = b.id - a.id;
    out.scope = n.getScope() === n; out.reference = n.updateReference() === n; out.hash = n.getHash(); out.uuid = n.uuid; return out;
}
function updates(r) {
    const s = state(r), n = s.root; n.version = decode(r.version, s, 'version');
    const values = [];
    for (const v of r.values) { try { n.needsUpdate = decode(v); values.push(encode(n.version)); } catch (e) { values.push('#' + e.name); } }
    const out = finish(s, values); out.needsOwn = Object.hasOwn(n, 'needsUpdate'); out.needsRead = encode(n.needsUpdate);
    n.constructor = {type: decode(r.type)}; out.type = encode(n.type);
    for (const key of ['updateType', 'updateBeforeType', 'updateAfterType', 'global']) n[key] = decode(r.type);
    out.accessors = [encode(n.getUpdateType()), encode(n.getUpdateBeforeType()), encode(n.getUpdateAfterType()), encode(n.isGlobal()), n.isCacheable(), n.getArrayCount(), n.getMemberType()]; return out;
}
function children(r) {
    const s = state(r), n = s.root, a = node('a', 101), b = node('b', 102);
    const list = [b, null, a], dict = {first: a, _private: b}; Object.defineProperty(dict, 'hidden', {value: b, writable: true, enumerable: false, configurable: true});
    n.direct = a; n.list = list; n.dictionary = dict; n.self = n;
    Object.defineProperty(n, 'hidden', {value: b, writable: true, enumerable: false, configurable: true});
    n.wrong = {isNode: 1}; n.nullPrototype = Object.assign(Object.create(null), {child: b});
    Object.defineProperty(n, '_private', {enumerable: true, configurable: true, get() { s.events.push(['privateGet']); return a; }});
    Object.defineProperty(n, 'dynamic', {enumerable: true, configurable: true, get() {
        s.events.push(['dynamicGet']); if (r.behavior === 'delete') delete n.tail; if (r.behavior === 'throw') throw new TypeError('child getter'); return b;
    }}); n.tail = a;
    if (r.behavior === 'array-self') list.push(n);
    const ignores = new Set(); if (r.ignore) ignores.add(a);
    const saved = Object.getOwnPropertyDescriptor(Object.prototype, 'nodeFixtureInherited');
    if (r.inherited) Object.defineProperty(Object.prototype, 'nodeFixtureInherited', {value: b, writable: true, enumerable: true, configurable: true});
    let result;
    try {
        if (r.generator) { const iterator = n.getChildren(); if (r.lazy) n.late = b; result = Array.from(iterator, v => normal(v)); }
        else result = attempt(n, '_getChildren', [ignores]);
    } catch (e) { result = '#' + e.name; }
    finally { if (r.inherited) { if (saved) Object.defineProperty(Object.prototype, 'nodeFixtureInherited', saved); else delete Object.prototype.nodeFixtureInherited; } }
    return {...finish(s, result), ignoredRoot: ignores.has(n)};
}
function cache(r) {
    const s = state(r), n = s.root, a = node('a', 101), b = node('b', 102); n.firstNode = a; n.secondNode = b; a.back = n;
    const results = [];
    for (let i = 0; i < 4; i++) { if (i === 1) a.version = 1; if (i === 2) n.needsUpdate = true; results.push(attempt(n, 'getCacheKey', [i === 0 ? decode(r.force) : false])); }
    return {...finish(s, results), version: n._cacheKeyVersion, key: encode(n._cacheKey)};
}
function types(r) {
    const s = state(r), n = s.root, b = builder(s); n.nodeType = decode(r.nodeType);
    if (r.custom) n.generateNodeType = fn(s, 'generateNodeType', 2);
    if (r.outputNode) { const o = node('output', 200); o.getNodeType = fn(s, 'generateNodeType', 2); entry(s, n).properties.outputNode = o; }
    const values = r.outputs.map(v => attempt(n, 'getNodeType', [b, decode(v)]));
    return {...finish(s, values), element: attempt(n, 'getElementType', [b]), data: normal(entry(s, n).data)};
}
function shared(r) {
    const s = state(r), n = s.root, b = builder(s), other = node('other', 200);
    if (r.mode === 'same') s.shared = n; if (r.mode === 'other') s.shared = other; if (r.mode === 'falsy') s.shared = 0;
    if (r.mode === 'override') { b.context.overrideNodes = new Map([[n, fn(s, 'overrideCallback', 1)]]); other.overrideNode = fn(s, 'overrideNode', 2); s.overrideNode = other; }
    return {...finish(s, Array.from({length: 3}, () => attempt(n, 'getShared', [b]))), data: normal(entry(s, n).data)};
}
function build(r) {
    const s = state(r), n = s.root, b = builder(s), d = entry(s, n).data, p = entry(s, n).properties;
    n.nodeType = 'float'; n.parents = !!r.parents;
    if (r.customSetup) n.setup = fn(s, 'setup', 1);
    if (r.customAnalyze) n.analyze = fn(s, 'analyze', 2);
    if (Object.hasOwn(r, 'generateArity')) n.generate = fn(s, 'generate', r.generateArity);
    if (Object.hasOwn(r, 'cacheable')) n.isCacheable = fn(s, 'isCacheable', 1);
    n.global = !!r.global;
    if (r.outputNode) { const o = node('output', 200); o.build = fn(s, 'outputBuild', 2); s.outputNode = o; if (!r.customSetup) p.outputNode = o; }
    if (r.children) { const child = node('child', 300); child.build = fn(s, 'childBuild', 2); child.parents = true; n.child = child; }
    if (r.before) { const before = node('before', 400); before.build = fn(s, 'beforeBuild', 2); n.before(before); }
    if (r.prebuilt) d.buildStages = {setup: true, analyze: true};
    if (Object.hasOwn(r, 'usage')) d.usageCount = decode(r.usage);
    if (Object.hasOwn(r, 'assign')) d.assign = decode(r.assign);
    if (Object.hasOwn(r, 'cachedSnippet')) d.snippet = decode(r.cachedSnippet);
    if (Object.hasOwn(r, 'cachedProperty')) d.propertyName = decode(r.cachedProperty);
    if (r.block) { const old = {parent: null, label: 'old'}, current = {label: 'current', parent: r.block === 'inside' ? old : null}; d.flowBlock = old; b.flowBlock = current; }
    const results = [];
    for (const step of r.steps) {
        b.buildStage = step.stage;
        const value = attempt(n, r.direct === undefined ? 'build' : r.direct, [b, decode(step.output)]);
        results.push({value, stage: b.buildStage, data: normal(d), properties: normal(p), chain: normal(s.chain)});
    }
    return {...finish(s, results), table: normal(s.table), flow: b.flow.code, beforeNodes: normal(n._beforeNodes)};
}
function bindings(r) {
    const s = state(r), n = s.root, argument = s.argument = {}, results = [], mode = r.mode;
    if (mode === 'dispose') {
        const listener = s.listener = fn(s, 'listener', 1); n.addEventListener('dispose', listener); n.addEventListener('dispose', listener);
        for (let i = 0; i < 2; i++) results.push(attempt(n, 'dispose'));
        results.push(encode(s.lastEvent.target), n.hasEventListener('dispose', listener));
    } else {
        const update = fn(s, mode === 'onReference' ? 'boundReference' : 'boundUpdate', 1);
        results.push(attempt(n, mode, mode === 'onUpdate' ? [update, decode(r.updateType)] : [update]));
        results.push(normal(n[mode === 'onReference' ? 'updateReference' : 'update'].call({}, argument)));
        results.push(encode(n.updateType));
    }
    return finish(s, results);
}
function traverse(r) {
    const s = state(r), root = s.root, a = node('a', 101), b = node('b', 102), c = node('c', 103);
    root.left = a; root.right = b; a.child = c; if (r.duplicate) root.again = a;
    function callback(n) {
        s.events.push(['visit', n.name, this === undefined]);
        if (r.throwAt === n.name) throw new TypeError('visit');
        if (r.mutate && n.name === 'root') { delete root.right; root.late = c; }
        return false;
    }
    return finish(s, attempt(root, 'traverse', [callback]));
}
function bindingOrder(r) {
    const s = state(r), root = s.root, callback = s.callback = {}, replacement = s.replacement = {};
    Object.defineProperty(callback, 'bind', {enumerable: true, configurable: true, get() {
        s.events.push(['getBind', this === callback]);
        if (r.throwAt === 'bindGet') throw new TypeError('bind');
        return function(...a) {
            s.events.push(['callBind', this === callback, a[0] === root, a.length]);
            if (r.throwAt === 'bindCall') throw new TypeError('bind');
            return replacement;
        };
    }});
    for (const name of ['updateType', 'update', 'updateReference']) Object.defineProperty(root, name, {enumerable: true, configurable: true, get: undefined, set(value) {
        s.events.push(['set', name, this === root, value === replacement ? '#replacement' : encode(value)]);
        if (r.throwAt === 'set:' + name) throw new TypeError('setter');
    }});
    return finish(s, attempt(root, r.operation, r.operation === 'onUpdate' ? [callback, 'custom'] : [callback]));
}
function observeGetter(s, object, key, label) {
    const value = object[key];
    Object.defineProperty(object, key, {enumerable: true, configurable: true, get() {
        s.events.push(['get', label, this === object]);
        if (s.request.throwAt === label) throw new TypeError('getter');
        return value;
    }});
}
function evaluationOrder(r) {
    const s = state(r), root = s.root, b = builder(s), d = entry(s, root).data, p = entry(s, root).properties;
    root.nodeType = 'float';
    const operation = r.operation, output = node('output', 200);
    output.build = fn(s, 'outputBuild', 2); output.getNodeType = fn(s, 'generateNodeType', 2);
    if (operation === 'generateNodeType' || operation === 'generate') p.outputNode = output;
    if (operation === 'build') {
        b.buildStage = 'generate'; d.buildStages = {setup: true, analyze: true}; d.usageCount = 1;
        if (r.cached) d.propertyName = 'existing'; root.generate = fn(s, 'generate', 2);
    }
    for (const name of ['getDataFromNode', 'getNodeProperties', 'getNodeFromHash', 'getVectorType', 'format', 'getElementType']) observeGetter(s, b, name, 'builder.' + name);
    for (const name of ['getNodeType', 'generateNodeType', 'getHash', 'isCacheable', 'generate']) observeGetter(s, root, name, 'node.' + name);
    observeGetter(s, root, 'nodeType', 'node.nodeType'); observeGetter(s, p, 'outputNode', 'properties.outputNode'); observeGetter(s, d, 'propertyName', 'data.propertyName');
    if (operation === 'serialize') {
        const child = node('child', 300); root.single = child;
        const json = {meta: {nodes: {}, textures: {}, images: {}}};
        observeGetter(s, json, 'meta', 'json.meta'); observeGetter(s, child, 'toJSON', 'child.toJSON');
        return finish(s, attempt(root, 'serialize', [json]));
    }
    return finish(s, attempt(root, operation, [b, 'vec3']));
}
function generator(r) {
    const s = state(r), root = s.root, records = s.records = [node('a', 101), node('b', 102)].map(childNode => ({childNode}));
    root._getChildren = function(...a) {
        s.events.push(['childrenCall', this === root, a.length]);
        if (r.reenter) s.events.push(['nested', attempt(s.iterator, r.nested, ['closed'])]);
        if (r.throwAt === 'children') throw new TypeError('children'); return records;
    };
    const iterator = s.iterator = root.getChildren(), result = [['created', s.events.length]];
    if (r.returnFirst) result.push(attempt(iterator, 'return', ['early']));
    for (let i = 0; i < 3; i++) result.push(attempt(iterator, 'next'));
    result.push(attempt(iterator, 'return', ['late'])); return finish(s, result);
}
function generatorCall(s, method, receiver, args = []) {
    try { return normal(Reflect.apply(method, receiver, args)); }
    catch (e) {
        if (e === s.token || e === s.closeToken) return {thrown: normal(e), sameToken: e === s.token, sameCloseToken: e === s.closeToken};
        if (e instanceof TypeError || e instanceof RangeError) return '#' + e.name;
        return {thrown: normal(e), sameToken: false, sameCloseToken: false};
    }
}
function generatorAttempt(s, iterator, method, args = []) { return generatorCall(s, iterator[method], iterator, args); }
function protocolGenerator(s, label) {
    const r = s.request, root = node(label, label === 'main' ? 400 : 500), inner = {}, iterable = {};
    const children = [node(label + '.a', 401), node(label + '.b', 402)];
    let index = 0;
    function nested(at) {
        if (r.nestedAt === at) s.events.push(['nested', at, generatorAttempt(s, s.iterator, r.nested, [s.token])]);
    }
    Object.defineProperty(inner, 'next', {configurable: true, get() {
        s.events.push(['nextGet', label, this === inner]);
        if (r.fault === 'nextGet') throw s.token;
        return function(...args) {
            s.events.push(['innerNext', label, this === inner, args.length]);
            if (r.fault === 'nextCall') throw s.token;
            if (r.fault === 'nextResult') return 5;
            const i = index++, record = {};
            Object.defineProperty(record, 'childNode', {get() {
                s.events.push(['child', label, i]);
                if (r.fault === 'childGet') throw s.token;
                return children[i];
            }});
            return {
                get done() { s.events.push(['done', label, i]); if (r.fault === 'doneGet') throw s.token; return i >= children.length; },
                get value() { s.events.push(['value', label, i]); if (r.fault === 'valueGet') throw s.token; return r.fault === 'childNull' ? null : record; }
            };
        };
    }});
    Object.defineProperty(inner, 'return', {configurable: true, get() {
        s.events.push(['returnGet', label, this === inner]); nested('get');
        if (r.close === 'getThrow') throw s.closeToken;
        if (r.close === 'missing') return undefined;
        if (r.close === 'null') return null;
        if (r.close === 'nonCallable') return 5;
        return function(...args) {
            s.events.push(['innerReturn', label, this === inner, args.length]); nested('call');
            if (r.close === 'callThrow') throw s.closeToken;
            if (r.close === 'primitive') return 5;
            return {value: 'inner-return', done: true};
        };
    }});
    inner.throw = function(...args) { s.events.push(['innerThrow', label, this === inner, args.length]); return {done: true}; };
    Object.defineProperty(iterable, Symbol.iterator, {configurable: true, get() {
        s.events.push(['iteratorGet', label, this === iterable]);
        if (r.fault === 'iteratorGet') throw s.token;
        return function(...args) {
            s.events.push(['iteratorCall', label, this === iterable, args.length]);
            if (r.fault === 'iteratorCall') throw s.token;
            return r.fault === 'iteratorResult' ? 5 : inner;
        };
    }});
    root._getChildren = function(...args) { s.events.push(['children', label, this === root, args.length]); return iterable; };
    return root.getChildren();
}
function generatorDescriptor(object, key) {
    const d = Object.getOwnPropertyDescriptor(object, key);
    return !d ? null : [Object.hasOwn(d, 'value') ? 'data' : 'accessor', encode(d.value), !!d.writable, !!d.enumerable, !!d.configurable, encode(d.get), encode(d.set)];
}
function generatorProtocol(r) {
    const s = state(r); s.token = {token: 'outer'}; s.closeToken = {token: 'close'};
    const iterator = s.iterator = protocolGenerator(s, 'main'), result = [];
    function advance(target, stage) {
        if (stage === 'suspended') result.push(['prepare', generatorAttempt(s, target, 'next')]);
        if (stage === 'closed') result.push(['prepare', generatorAttempt(s, target, 'return', ['prepared'])]);
        if (stage === 'exhausted') for (let i = 0; i < 3; i++) result.push(['prepare', generatorAttempt(s, target, 'next')]);
    }
    if (r.mode === 'metadata') {
        const other = protocolGenerator(s, 'other'), method = Node.prototype.getChildren;
        const p = Object.getPrototypeOf(iterator), common = Object.getPrototypeOf(p);
        const names = object => Object.getOwnPropertyNames(object), symbols = object => Object.getOwnPropertySymbols(object).map(v => v.description);
        const constructible = fn => { try { Reflect.construct(Object, [], fn); return true; } catch { return false; } };
        result.push({iteratorOwn: names(iterator), iteratorSymbols: symbols(iterator), prototypeOwn: names(p), prototypeSymbols: symbols(p),
            prototypeIsMethodPrototype: p === method.prototype, commonOwn: names(common), commonSymbols: symbols(common),
            tag: encode(iterator[Symbol.toStringTag]), tagDescriptor: generatorDescriptor(common, Symbol.toStringTag),
            methodOwn: names(method), methodPrototypeDescriptor: generatorDescriptor(method, 'prototype'), methodConstructible: constructible(method),
            iteratorReturnsSelf: iterator[Symbol.iterator]() === iterator,
            genericIteratorReturnsReceiver: Reflect.apply(iterator[Symbol.iterator], s.token, []) === s.token});
        for (const key of ['next', 'return', 'throw']) {
            const fn = iterator[key]; let owner = iterator, depth = 0;
            while (owner !== null && !Object.hasOwn(owner, key)) { owner = Object.getPrototypeOf(owner); depth++; }
            result.push([key, Object.hasOwn(iterator, key), fn === other[key], depth, generatorDescriptor(owner, key), names(fn), fn.name, fn.length,
                constructible(fn), Object.getPrototypeOf(fn) === Object.getPrototypeOf(Node.prototype.getHash)]);
        }
    } else if (r.mode === 'brand') {
        advance(iterator, r.stage);
        const receivers = {object: {}, directSpoof: Object.create(Object.getPrototypeOf(iterator)), commonSpoof: Object.create(Object.getPrototypeOf(Object.getPrototypeOf(iterator))),
            array: [], node: s.root, null: null, undefined: undefined, number: 1, function() {}};
        result.push(['borrowed', generatorCall(s, iterator[r.method], receivers[r.receiver], [s.token])]);
        result.push(['original', generatorAttempt(s, iterator, 'next')]);
    } else if (r.mode === 'borrow') {
        const other = protocolGenerator(s, 'other'); advance(iterator, r.stage); advance(other, r.otherStage);
        result.push(['borrowed', generatorCall(s, iterator[r.method], other, [s.token])]);
        result.push(['original', generatorAttempt(s, iterator, 'next')]); result.push(['other', generatorAttempt(s, other, 'next')]);
    } else {
        advance(iterator, r.stage);
        result.push(['action', generatorAttempt(s, iterator, r.method, [s.token])]);
        for (const method of ['next', 'return', 'throw', 'next']) result.push(['after', method, generatorAttempt(s, iterator, method, [s.token])]);
    }
    return finish(s, result);
}
function enumerationMutation(r) {
    const s = state(r), dictionary = {}, a = node('a', 101), b = node('b', 102), c = node('c', 103);
    const key = 'nodeParityMutation', prototype = Object.prototype, saved = Object.getOwnPropertyDescriptor(prototype, key);
    const define = (object, value, enumerable = true) => Object.defineProperty(object, key, {value, writable: true, enumerable, configurable: true});
    Object.defineProperty(dictionary, 'first', {enumerable: true, configurable: true, get() {
        s.events.push(['getFirst', this === dictionary]);
        if (r.mode === 'addPrototype') define(prototype, b);
        if (r.mode === 'deletePrototype') delete prototype[key];
        if (r.mode === 'hidePrototype') Object.defineProperty(prototype, key, {enumerable: false});
        if (r.mode === 'revealPrototype') Object.defineProperty(prototype, key, {enumerable: true});
        if (r.mode === 'replacePrototypeValue') prototype[key] = c;
        if (r.mode === 'addOwn') define(dictionary, b);
        if (r.mode === 'deleteOwn' || r.mode === 'deleteShadow') delete dictionary[key];
        if (r.mode === 'hideOwn') Object.defineProperty(dictionary, key, {enumerable: false});
        if (r.mode === 'revealOwn') Object.defineProperty(dictionary, key, {enumerable: true});
        return a;
    }});
    s.root.dictionary = dictionary;
    let result;
    try {
        if (['deletePrototype', 'hidePrototype', 'revealPrototype', 'replacePrototypeValue', 'deleteShadow'].includes(r.mode)) define(prototype, b, r.mode !== 'revealPrototype');
        if (['deleteOwn', 'hideOwn', 'revealOwn', 'deleteShadow'].includes(r.mode)) define(dictionary, b, !['revealOwn', 'deleteShadow'].includes(r.mode));
        result = attempt(s.root, '_getChildren');
    } finally { if (saved) Object.defineProperty(prototype, key, saved); else delete prototype[key]; }
    return finish(s, result);
}
function serialization(r) {
    const s = state(r), n = s.root, a = node('a', 101), b = node('b', 102); n.single = a; n.list = [a, b]; n.dictionary = {first: b};
    const meta = {nodes: {}, textures: {}, images: {}};
    if (r.mode === 'serialize') { const json = {meta}; const result = attempt(n, 'serialize', [json]); return {...finish(s, result), json: normal(json)}; }
    if (r.mode === 'deserialize') {
        const json = {inputNodes: {single: 'uuid-a', list: ['uuid-a', 'missing', 'uuid-b'], dictionary: {x: 'uuid-b'}}, meta};
        meta.nodes['uuid-a'] = a; meta.nodes['uuid-b'] = b;
        const result = attempt(n, 'deserialize', [json]); return {...finish(s, result), fields: [normal(n.single), normal(n.list), normal(n.dictionary)]};
    }
    const values = [];
    if (r.mode === 'root') values.push(attempt(n, 'toJSON'));
    else if (r.mode === 'string') values.push(attempt(n, 'toJSON', ['root']));
    else if (r.mode === 'null') values.push(attempt(n, 'toJSON', [null]));
    else { values.push(attempt(n, 'toJSON', [meta])); values.push(attempt(n, 'toJSON', [meta])); }
    return {...finish(s, values), meta: normal(meta)};
}
function run(r) {
    if (r.kind === 'metadata') return metadata(r); if (r.kind === 'updates') return updates(r); if (r.kind === 'children') return children(r);
    if (r.kind === 'cache') return cache(r); if (r.kind === 'types') return types(r); if (r.kind === 'shared') return shared(r);
    if (r.kind === 'build') return build(r); if (r.kind === 'bindings') return bindings(r); if (r.kind === 'serialization') return serialization(r);
    if (r.kind === 'traverse') return traverse(r); if (r.kind === 'bindingOrder') return bindingOrder(r);
    if (r.kind === 'evaluationOrder') return evaluationOrder(r); if (r.kind === 'generator') return generator(r);
    if (r.kind === 'generatorProtocol') return generatorProtocol(r);
    if (r.kind === 'enumerationMutation') return enumerationMutation(r);
    throw new TypeError('unknown Node scenario');
}
const cases = [{kind: 'metadata'}];
for (const version of [0, 4, '4', null, false, {tag: 'undefined'}, {tag: 'nan'}, {tag: 'coerce', value: 8}, {tag: 'symbol'}])
    for (const type of ['vec3', null, 0, {tag: 'undefined'}])
        cases.push({kind: 'updates', version, type, values: [false, 1, 'true', {}, null, {tag: 'undefined'}, true, true]});
for (const behavior of ['', 'delete', 'throw', 'array-self']) for (const generator of [false, true])
    for (const ignore of [false, true]) for (const inherited of [false, true])
        cases.push({kind: 'children', behavior, generator, ignore, inherited, lazy: true});
for (const force of [false, true, 1, 'force', null, {tag: 'undefined'}]) cases.push({kind: 'cache', force});
for (const typeResult of ['float', null, false, {tag: 'undefined'}]) for (const custom of [false, true]) for (const outputNode of [false, true])
    cases.push({kind: 'types', nodeType: 'vec3', typeResult, custom, outputNode, outputs: [null, null, 'float', 'float', 'vec4', '__proto__', 'toString', null]});
for (const mode of ['', 'same', 'other', 'falsy', 'override']) for (const reenter of [false, true]) for (const falsyOverride of [false, true])
    for (const throwAt of ['', 'overrideCallback', 'overrideNode']) cases.push({kind: 'shared', mode, reenter, falsyOverride, throwAt});
for (const direct of ['setup', 'analyze', 'generate']) for (const children of [false, true]) for (const outputNode of [false, true])
    cases.push({kind: 'build', direct, children, outputNode, parents: true, steps: [{stage: direct, output: 'float'}, {stage: direct, output: 'vec3'}]});
for (const stage of ['setup', 'analyze', 'generate', 'unknown']) for (const before of [false, true]) for (const children of [false, true])
    cases.push({kind: 'build', before, children, customSetup: true, outputNode: true, parents: true,
        steps: [{stage, output: 'float'}, {stage, output: 'vec3'}]});
for (const generateArity of [0, 1, 2]) for (const usage of [0, 1, 2, 3]) for (const output of [null, 'float', 'void', 'reference', 'OutputType'])
    cases.push({kind: 'build', generateArity, usage, prebuilt: true, steps: [{stage: 'generate', output}, {stage: 'generate', output: 'vec3'}]});
for (const cacheable of [false, true, 0, 1, null]) for (const assign of [false, true, 1])
    cases.push({kind: 'build', generateArity: 1, usage: 3, prebuilt: true, cacheable, assign, steps: [{stage: 'generate', output: 'vec3'}]});
for (const block of ['inside', 'outside']) for (const global of [false, true]) for (const emitFlow of [false, true])
    cases.push({kind: 'build', generateArity: 1, prebuilt: true, usage: 1, block, global, emitFlow, cachedSnippet: 'old-code', cachedProperty: 'old-var', steps: [{stage: 'generate', output: 'float'}]});
for (const snippet of ['', false, null, {tag: 'undefined'}]) for (const generateArity of [1, 2])
    cases.push({kind: 'build', generateArity, prebuilt: true, usage: 1, snippet, steps: [{stage: 'generate', output: 'float'}]});
cases.push({kind: 'build', generateArity: 1, prebuilt: true, usage: 1, reenterGenerate: true, steps: [{stage: 'generate', output: 'float'}]});
cases.push({kind: 'build', customSetup: true, reenterSetup: true, steps: [{stage: 'setup', output: null}]});
for (const throwAt of ['getNodeFromHash', 'getDataFromNode', 'setBuildStage', 'addChain', 'addNode', 'setup', 'childBuild', 'addSequentialNode', 'increaseUsage', 'getVectorType', 'generate', 'format', 'getVarFromNode', 'addLineFlowCode', 'removeChain', 'beforeBuild'])
    cases.push({kind: 'build', generateArity: 1, usage: 3, before: true, children: true, customSetup: true, throwAt,
        steps: [{stage: 'generate', output: 'float'}, {stage: 'generate', output: 'float'}]});
for (const mode of ['onUpdate', 'onFrameUpdate', 'onRenderUpdate', 'onObjectUpdate', 'onReference'])
    cases.push({kind: 'bindings', mode, updateType: 'custom'});
for (const removeListener of [false, true]) cases.push({kind: 'bindings', mode: 'dispose', removeListener});
for (const mode of ['serialize', 'deserialize', 'root', 'string', 'null', 'meta']) cases.push({kind: 'serialization', mode});
for (const duplicate of [false, true]) for (const mutate of [false, true]) for (const throwAt of ['', 'root', 'a', 'c', 'b'])
    cases.push({kind: 'traverse', duplicate, mutate, throwAt});
for (const operation of ['onUpdate', 'onFrameUpdate', 'onRenderUpdate', 'onObjectUpdate', 'onReference'])
    for (const throwAt of ['', 'bindGet', 'bindCall', 'set:updateType', 'set:update', 'set:updateReference'])
        cases.push({kind: 'bindingOrder', operation, throwAt});
for (const operation of ['getElementType', 'getNodeType', 'generateNodeType', 'getShared', 'generate', 'build', 'serialize'])
    for (const cached of [false, true]) for (const throwAt of ['', 'builder.getVectorType', 'builder.format', 'json.meta', 'child.toJSON'])
        cases.push({kind: 'evaluationOrder', operation, cached, throwAt, typeResult: 'vec4'});
for (const reenter of [false, true]) for (const nested of ['next', 'return']) for (const returnFirst of [false, true])
    for (const throwAt of ['', 'children']) cases.push({kind: 'generator', reenter, nested, returnFirst, throwAt});
cases.push({kind: 'generatorProtocol', mode: 'metadata'});
for (const method of ['next', 'return', 'throw']) for (const stage of ['before', 'suspended', 'closed'])
    for (const receiver of ['object', 'directSpoof', 'commonSpoof', 'array', 'node', 'null', 'undefined', 'number', 'function'])
        cases.push({kind: 'generatorProtocol', mode: 'brand', method, stage, receiver});
for (const method of ['next', 'return', 'throw']) for (const stage of ['before', 'suspended', 'closed'])
    for (const otherStage of ['before', 'suspended', 'closed']) cases.push({kind: 'generatorProtocol', mode: 'borrow', method, stage, otherStage});
for (const method of ['return', 'throw']) for (const stage of ['before', 'suspended', 'exhausted'])
    for (const close of ['', 'getThrow', 'callThrow', 'primitive', 'missing', 'null', 'nonCallable'])
        cases.push({kind: 'generatorProtocol', mode: 'close', method, stage, close});
for (const method of ['return', 'throw']) for (const nestedAt of ['get', 'call']) for (const nested of ['next', 'return', 'throw'])
    for (const close of ['', 'getThrow', 'callThrow']) cases.push({kind: 'generatorProtocol', mode: 'close', method, stage: 'suspended', nestedAt, nested, close});
for (const fault of ['iteratorGet', 'iteratorCall', 'iteratorResult', 'nextGet', 'nextCall', 'nextResult', 'doneGet', 'valueGet', 'childGet', 'childNull'])
    for (const close of ['', 'getThrow', 'callThrow', 'primitive']) cases.push({kind: 'generatorProtocol', mode: 'fault', method: 'next', stage: 'before', fault, close});
for (const mode of ['addPrototype', 'deletePrototype', 'hidePrototype', 'revealPrototype', 'replacePrototypeValue', 'addOwn', 'deleteOwn', 'hideOwn', 'revealOwn', 'deleteShadow'])
    cases.push({kind: 'enumerationMutation', mode});

// Console warnings are diagnostic output, not expected data. Keep their actual
// emission out of JSON; method effects remain observable in events and records.
const oldWarn = console.warn, oldError = console.error;
let expected;
try { console.warn = () => {}; console.error = () => {}; expected = cases.map(run); }
finally { console.warn = oldWarn; console.error = oldError; }
if (outputDirectory) {
    fs.mkdirSync(outputDirectory, {recursive: true});
    fs.writeFileSync(path.join(outputDirectory, 'node-input.json'), JSON.stringify(cases, null, 2));
    fs.writeFileSync(path.join(outputDirectory, 'node-expected.json'), JSON.stringify(expected, null, 2));
}
if (binary) {
    const actual = JSON.parse(execFileSync(binary, [], {input: JSON.stringify(cases), encoding: 'utf8', maxBuffer: 256 * 1024 * 1024}));
    const differences = [];
    assert.equal(actual.length, expected.length);
    for (let i = 0; i < cases.length; i++) { try { assert.deepEqual(actual[i], expected[i]); } catch { differences.push({index: i, request: cases[i], expected: expected[i], actual: actual[i]}); } }
    if (outputDirectory) {
        fs.writeFileSync(path.join(outputDirectory, 'node-actual.json'), JSON.stringify(actual, null, 2));
        fs.writeFileSync(path.join(outputDirectory, 'node-differences.json'), JSON.stringify(differences, null, 2));
    }
    assert.equal(differences.length, 0, `${differences.length} Node differences; first ${JSON.stringify(differences[0])}`);
}
console.log(`Node ${binary ? 'dynamic protocol parity' : 'actual upstream oracle'}: ${cases.length} scenarios`);
assert.equal(typeof global.gc, 'function', 'Use node --expose-gc');
let live, weakNode, weakCallback, weakPayload;
function setupGC() {
    const n = node(), payload = {}; payload.self = payload;
    const cb = () => payload; n.onFrameUpdate(cb); n.self = n; payload.node = n;
    weakNode = new WeakRef(n); weakCallback = new WeakRef(cb); weakPayload = new WeakRef(payload); live = n;
}
async function jobs() { await new Promise(setImmediate); await new Promise(setImmediate); global.gc(); await new Promise(setImmediate); global.gc(); }
setupGC(); await jobs(); assert.equal(weakNode.deref(), live);
const expectedGC = [[!!weakNode.deref(), !!weakCallback.deref(), !!weakPayload.deref()]];
live = undefined; await jobs(); expectedGC.push([!!weakNode.deref(), !!weakCallback.deref(), !!weakPayload.deref()]);
let generatorSource, generatorWeak;
function gcGeneratorChildren() { return generatorSource; }
function gcGeneratorNext() { return function() {
    if (this.mode === 'nextThrow') throw 'err';
    return {value: this.mode === 'childThrow' ? null : {childNode: 'child'}, done: this.index++ > 0};
}; }
function gcGeneratorIterable() { return this.inner; }
function gcGeneratorReturn() { return {done: true}; }
function setupGeneratorGC(stage) {
    const n = node(), next = gcGeneratorNext(), inner = {index: 0, mode: stage, next, return: gcGeneratorReturn};
    const iterable = {inner, [Symbol.iterator]: gcGeneratorIterable};
    n._getChildren = gcGeneratorChildren; generatorSource = iterable; live = n.getChildren();
    generatorWeak = [n, iterable, inner, next].map(value => new WeakRef(value));
    try {
        if (stage !== 'before') live.next();
        if (stage === 'exhausted') live.next();
        if (stage === 'returned') live.return();
        if (stage === 'thrown') live.throw('outer');
    } catch {}
    generatorSource = undefined;
}
for (const stage of ['before', 'suspended', 'exhausted', 'returned', 'thrown', 'nextThrow', 'childThrow']) {
    setupGeneratorGC(stage); await jobs(); expectedGC.push(generatorWeak.map(value => !!value.deref()));
    live = undefined; await jobs(); expectedGC.push(generatorWeak.map(value => !!value.deref()));
}
const actualGC = binary ? JSON.parse(execFileSync(binary, ['gc'], {encoding: 'utf8'})) : undefined;
if (outputDirectory) {
    fs.writeFileSync(path.join(outputDirectory, 'node-gc-expected.json'), JSON.stringify(expectedGC, null, 2));
    if (binary) fs.writeFileSync(path.join(outputDirectory, 'node-gc-actual.json'), JSON.stringify(actualGC, null, 2));
}
if (binary) assert.deepEqual(actualGC, expectedGC);
console.log(`Node ${binary ? 'actual Node/native GC parity' : 'actual Node GC oracle'}: ${expectedGC.length} snapshots`);
